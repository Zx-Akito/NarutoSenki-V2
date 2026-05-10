#!/usr/bin/env bash
set -euo pipefail

# Build NarutoSenki Android APK from CLI.
# Requires Android SDK (and NDK for native code). Gradle reads sdk.dir from
# local.properties, or this script can create it from ANDROID_HOME / ANDROID_SDK_ROOT.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

if [[ ! -f gradlew ]]; then
  echo "Error: gradlew not found in ${SCRIPT_DIR}" >&2
  exit 1
fi

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
  cat <<'EOF'
Usage: ./build-android.sh [COMMAND]

  (no args)     ./gradlew assembleDebug
  debug         assembleDebug
  release       assembleRelease
  clean         clean
  install       installDebug (needs device/emulator)

Any other COMMAND is passed through to Gradle, e.g.:
  ./build-android.sh assembleRelease bundleRelease --stacktrace

APK output: app/build/outputs/apk/<variant>/
Requires: Android SDK (+ NDK). JDK 8–12 for Gradle 5.6 (JDK 17+ will fail).
  Override: GRADLE_JAVA_HOME=/path/to/jdk11 ./build-android.sh

AGP 3.3 needs a classic NDK (with ndk.dir/platforms). NDK r23+ will not work.
  Install: sdkmanager \"ndk;21.4.7075529\" then re-run this script.
EOF
  exit 0
fi

# Gradle 5.6.x + Groovy 2.5 break on JDK 17+ (NoClassDefFoundError: Java7 VM plugin).
_java_major_version() {
  local line j="${1:-java}"
  line="$("$j" -version 2>&1 | head -n1 || true)"
  if [[ "$line" =~ version\ \"1\.([0-9]+) ]]; then
    echo "${BASH_REMATCH[1]}"
  elif [[ "$line" =~ version\ \"([0-9]+) ]]; then
    echo "${BASH_REMATCH[1]}"
  else
    echo 99
  fi
}

ensure_gradle_jdk() {
  if [[ -n "${GRADLE_JAVA_HOME:-}" ]]; then
    export JAVA_HOME="$GRADLE_JAVA_HOME"
    local gmaj
    gmaj="$(_java_major_version "${JAVA_HOME}/bin/java")"
    if [[ "$gmaj" -gt 12 ]]; then
      echo "Error: GRADLE_JAVA_HOME must be JDK 8–12 (this one is Java $gmaj)." >&2
      exit 1
    fi
    return 0
  fi

  local java_exe=java
  if [[ -n "${JAVA_HOME:-}" && -x "${JAVA_HOME}/bin/java" ]]; then
    java_exe="${JAVA_HOME}/bin/java"
  fi

  local major
  major="$(_java_major_version "$java_exe")"

  if [[ "$major" -le 12 ]]; then
    return 0
  fi

  # java_home -v can return the wrong JDK; only accept JAVA_HOME after verifying java -version.
  try_java_home_candidate() {
    local jh="$1"
    [[ -n "$jh" && -x "$jh/bin/java" ]] || return 1
    local mj
    mj="$(_java_major_version "$jh/bin/java")"
    [[ "$mj" -le 12 ]] || return 1
    export JAVA_HOME="$jh"
    echo "Using JAVA_HOME=$JAVA_HOME (Java $mj for Gradle 5.6; JVM on PATH was Java $major)." >&2
    return 0
  }

  local jh paths
  if [[ -x /usr/libexec/java_home ]]; then
    for v in 11 12 1.8; do
      jh="$(/usr/libexec/java_home -v "$v" 2>/dev/null)" || continue
      try_java_home_candidate "$jh" && return 0
    done
  fi

  paths=(
    "/opt/homebrew/opt/openjdk@11/libexec/openjdk.jdk/Contents/Home"
    "/usr/local/opt/openjdk@11/libexec/openjdk.jdk/Contents/Home"
    "/Library/Java/JavaVirtualMachines/temurin-11.jdk/Contents/Home"
    "/Library/Java/JavaVirtualMachines/microsoft-11.jdk/Contents/Home"
    "/Library/Java/JavaVirtualMachines/liberica-jdk-11.jdk/Contents/Home"
    "/Library/Java/JavaVirtualMachines/amazon-corretto-11.jdk/Contents/Home"
  )
  for jh in "${paths[@]}"; do
    try_java_home_candidate "$jh" && return 0
  done

  echo "Error: Gradle 5.6 needs JDK 8–12. JVM on PATH/Java_HOME is Java $major (too new)." >&2
  echo "Install JDK 11, e.g. brew install --cask temurin11   or   brew install openjdk@11" >&2
  echo "Then: GRADLE_JAVA_HOME=\$(/usr/libexec/java_home -v 11) ./build-android.sh" >&2
  exit 1
}

chmod +x ./gradlew 2>/dev/null || true

if [[ ! -f local.properties ]]; then
  _sdk=""
  if [[ -n "${ANDROID_HOME:-}" ]]; then
    _sdk="$ANDROID_HOME"
  elif [[ -n "${ANDROID_SDK_ROOT:-}" ]]; then
    _sdk="$ANDROID_SDK_ROOT"
  fi
  if [[ -n "$_sdk" ]]; then
    printf 'sdk.dir=%s\n' "$_sdk" > local.properties
    echo "Created local.properties with sdk.dir=${_sdk}"
  else
    echo "Error: missing local.properties and ANDROID_HOME / ANDROID_SDK_ROOT is unset." >&2
    echo "Add local.properties with one line: sdk.dir=/path/to/Android/sdk" >&2
    exit 1
  fi
fi

ensure_gradle_jdk

# Android Gradle Plugin 3.3 checks for $NDK/platforms — that folder was removed in NDK r23+.
_classic_ndk_ok() {
  [[ -n "${1:-}" && -d "$1" && -d "$1/platforms" && -x "$1/ndk-build" ]]
}

_ndk_major_version() {
  local ndk="$1" rev
  rev="$(awk -F= '/^Pkg\.Revision/{print $2}' "$ndk/source.properties" 2>/dev/null | tr -d '[:space:]' || true)"
  [[ -n "$rev" ]] || { echo 0; return 0; }
  echo "${rev%%.*}"
}

_host_needs_modern_ndk() {
  [[ "$(uname -s)" == "Darwin" && "$(uname -m)" == "arm64" ]]
}

_ndk_host_compatible() {
  local ndk="$1" major
  major="$(_ndk_major_version "$ndk")"
  if _host_needs_modern_ndk && [[ "$major" -gt 0 && "$major" -lt 21 ]]; then
    return 1
  fi
  return 0
}

_prepare_ndk_agp33_compat() {
  local ndk="$1"
  local host_tag=""
  [[ -n "$ndk" && -d "$ndk" && -x "$ndk/ndk-build" ]] || return 1

  # AGP 3.3 checks for $NDK/platforms. Newer NDK removed it, so create a harmless shim.
  if [[ ! -d "$ndk/platforms" ]]; then
    mkdir -p "$ndk/platforms/android-21/arch-arm/usr/include" \
      "$ndk/platforms/android-21/arch-arm64/usr/include" 2>/dev/null || return 1
  fi

  # AGP 3.3 may also probe legacy GCC toolchain folder names even when clang is used.
  case "$(uname -s)-$(uname -m)" in
    Darwin-arm64|Darwin-x86_64) host_tag="darwin-x86_64" ;;
    Linux-x86_64) host_tag="linux-x86_64" ;;
    *) host_tag="" ;;
  esac
  if [[ -n "$host_tag" ]]; then
    local llvm_strip=""
    llvm_strip="$ndk/toolchains/llvm/prebuilt/$host_tag/bin/llvm-strip"
    mkdir -p "$ndk/toolchains/arm-linux-androideabi-4.9/prebuilt/$host_tag" 2>/dev/null || true
    mkdir -p "$ndk/toolchains/aarch64-linux-android-4.9/prebuilt/$host_tag" 2>/dev/null || true
    mkdir -p "$ndk/toolchains/x86-4.9/prebuilt/$host_tag" 2>/dev/null || true
    mkdir -p "$ndk/toolchains/x86_64-4.9/prebuilt/$host_tag" 2>/dev/null || true
    mkdir -p "$ndk/toolchains/mipsel-linux-android-4.9/prebuilt/$host_tag" 2>/dev/null || true
    mkdir -p "$ndk/toolchains/mips64el-linux-android-4.9/prebuilt/$host_tag" 2>/dev/null || true
    if [[ -x "$llvm_strip" ]]; then
      mkdir -p "$ndk/toolchains/arm-linux-androideabi-4.9/prebuilt/$host_tag/bin" 2>/dev/null || true
      mkdir -p "$ndk/toolchains/aarch64-linux-android-4.9/prebuilt/$host_tag/bin" 2>/dev/null || true
      mkdir -p "$ndk/toolchains/x86-4.9/prebuilt/$host_tag/bin" 2>/dev/null || true
      mkdir -p "$ndk/toolchains/x86_64-4.9/prebuilt/$host_tag/bin" 2>/dev/null || true
      ln -sf "$llvm_strip" "$ndk/toolchains/arm-linux-androideabi-4.9/prebuilt/$host_tag/bin/arm-linux-androideabi-strip"
      ln -sf "$llvm_strip" "$ndk/toolchains/aarch64-linux-android-4.9/prebuilt/$host_tag/bin/aarch64-linux-android-strip"
      ln -sf "$llvm_strip" "$ndk/toolchains/x86-4.9/prebuilt/$host_tag/bin/i686-linux-android-strip"
      ln -sf "$llvm_strip" "$ndk/toolchains/x86_64-4.9/prebuilt/$host_tag/bin/x86_64-linux-android-strip"
    fi
  fi
  return 0
}

_ndk_agp33_compatible() {
  local ndk="$1"
  [[ -n "$ndk" && -d "$ndk" && -x "$ndk/ndk-build" ]] || return 1
  _ndk_host_compatible "$ndk" || return 1
  _prepare_ndk_agp33_compat "$ndk" || return 1
  [[ -d "$ndk/platforms" ]]
}

# Prefer newest side-by-side NDK that still has platforms/ (e.g. 21.x).
_find_classic_ndk_under_root() {
  local root="$1"
  local d
  [[ -n "$root" && -d "$root/ndk" ]] || return 1
  while IFS= read -r d; do
    [[ -z "$d" ]] && continue
    if _ndk_agp33_compatible "$d"; then
      echo "$d"
      return 0
    fi
  done < <(find "$root/ndk" -mindepth 1 -maxdepth 1 -type d 2>/dev/null | sort -Vr)
  return 1
}

_find_sdkmanager() {
  local c
  for c in \
    "/Users/zxakito/Library/Android/sdk/cmdline-tools/latest/bin/sdkmanager" \
    "/Users/zxakito/Library/Android/sdk/tools/bin/sdkmanager" \
    "/opt/homebrew/share/android-commandlinetools/cmdline-tools/latest/bin/sdkmanager" \
    "/opt/homebrew/share/android-commandlinetools/tools/bin/sdkmanager"; do
    if [[ -x "$c" ]]; then
      echo "$c"
      return 0
    fi
  done
  return 1
}

_try_install_classic_ndk() {
  local sdkmanager_bin="$(_find_sdkmanager || true)"
  local sdk_java_home=""
  if [[ -z "$sdkmanager_bin" ]]; then
    return 1
  fi

  echo "Classic NDK not found. Trying auto-install: ndk;21.4.7075529"

  # New cmdline-tools require JDK 17+, while Gradle build here needs JDK 11.
  if [[ -x "/Library/Java/JavaVirtualMachines/temurin-17.jdk/Contents/Home/bin/java" ]]; then
    sdk_java_home="/Library/Java/JavaVirtualMachines/temurin-17.jdk/Contents/Home"
  elif [[ -x "/opt/homebrew/opt/openjdk/bin/java" ]]; then
    sdk_java_home="/opt/homebrew/opt/openjdk"
  fi

  if [[ -n "$sdk_java_home" ]]; then
    yes | env JAVA_HOME="$sdk_java_home" "$sdkmanager_bin" "ndk;21.4.7075529" >/dev/null 2>&1 || return 1
    # Optional extra fallback: NDK r16 has platforms/ and can satisfy AGP checks on some hosts.
    yes | env JAVA_HOME="$sdk_java_home" "$sdkmanager_bin" "ndk;16.1.4479499" >/dev/null 2>&1 || true
  else
    yes | "$sdkmanager_bin" "ndk;21.4.7075529" >/dev/null 2>&1 || return 1
  fi
  return 0
}

_strip_ndk_entries_from_local_properties() {
  [[ -f local.properties ]] || return 0
  awk '!/^ndk\.dir=/' local.properties > local.properties.tmp \
    && mv local.properties.tmp local.properties
}

_merge_ndk_into_local_properties() {
  local _ndk="$1"
  if [[ -f local.properties ]]; then
    if awk '/^ndk\.dir=/' local.properties >/dev/null 2>&1; then
      awk -v ndk="$_ndk" '
        BEGIN { updated=0 }
        /^ndk\.dir=/ { print "ndk.dir=" ndk; updated=1; next }
        { print }
        END { if (!updated) print "ndk.dir=" ndk }
      ' local.properties > local.properties.tmp && mv local.properties.tmp local.properties
    else
      printf 'ndk.dir=%s\n' "$_ndk" >> local.properties
    fi
  else
    printf 'ndk.dir=%s\n' "$_ndk" > local.properties
  fi
  echo "Using ndk.dir=${_ndk}"
}

ensure_ndk_configured() {
  local _ndk="" _sdk="" d

  if [[ -f local.properties ]]; then
    _ndk="$(grep -E '^ndk\.dir=' local.properties 2>/dev/null | head -n1 | cut -d= -f2- || true)"
    _sdk="$(grep -E '^sdk\.dir=' local.properties 2>/dev/null | head -n1 | cut -d= -f2- || true)"
  fi

  if _ndk_agp33_compatible "$_ndk"; then
    return 0
  fi

  if [[ -n "$_ndk" ]] && ! _ndk_agp33_compatible "$_ndk"; then
    echo "Removing incompatible ndk.dir (AGP 3.3 needs NDK 21-style layout with platforms/)." >&2
    _strip_ndk_entries_from_local_properties
    _ndk=""
  fi

  for d in "${ANDROID_NDK_HOME:-}" "${ANDROID_NDK_ROOT:-}"; do
    if _ndk_agp33_compatible "$d"; then
      _merge_ndk_into_local_properties "$d"
      return 0
    fi
  done

  if [[ -n "$_sdk" && -d "$_sdk/ndk-bundle" ]] && _ndk_agp33_compatible "$_sdk/ndk-bundle"; then
    _merge_ndk_into_local_properties "$_sdk/ndk-bundle"
    return 0
  fi

  d="$(_find_classic_ndk_under_root "${HOME}/Library/Android/sdk" || true)"
  if _ndk_agp33_compatible "$d"; then
    _merge_ndk_into_local_properties "$d"
    return 0
  fi

  d="$(_find_classic_ndk_under_root "${_sdk}" || true)"
  if _ndk_agp33_compatible "$d"; then
    _merge_ndk_into_local_properties "$d"
    return 0
  fi

  d="$(_find_classic_ndk_under_root "/opt/homebrew/share/android-commandlinetools" || true)"
  if _ndk_agp33_compatible "$d"; then
    _merge_ndk_into_local_properties "$d"
    return 0
  fi

  # Optional self-heal path: install an AGP-3.3-compatible NDK automatically.
  if _try_install_classic_ndk; then
    d="$(_find_classic_ndk_under_root "${HOME}/Library/Android/sdk" || true)"
    if _ndk_agp33_compatible "$d"; then
      _merge_ndk_into_local_properties "$d"
      return 0
    fi
    d="$(_find_classic_ndk_under_root "${_sdk}" || true)"
    if _ndk_agp33_compatible "$d"; then
      _merge_ndk_into_local_properties "$d"
      return 0
    fi
    d="$(_find_classic_ndk_under_root "/opt/homebrew/share/android-commandlinetools" || true)"
    if _ndk_agp33_compatible "$d"; then
      _merge_ndk_into_local_properties "$d"
      return 0
    fi
  fi

  echo "Error: Android Gradle Plugin 3.3 needs an NDK with a platforms directory (NDK r21.x layout)." >&2
  echo "NDK r23+ side-by-side packages omit platforms/ at the root, so this Gradle version rejects them." >&2
  echo "Install NDK 21 side-by-side, then re-run:" >&2
  cat <<'EOT' >&2
  SDK="$HOME/Library/Android/sdk"
  "$SDK/cmdline-tools/latest/bin/sdkmanager" "ndk;21.4.7075529" || \
  "$SDK/tools/bin/sdkmanager" "ndk;21.4.7075529"
EOT
  echo "Then ndk.dir will look like: \$SDK/ndk/21.4.7075529" >&2
  echo "Or build with: ANDROID_NDK_HOME=\"\$HOME/Library/Android/sdk/ndk/21.4.7075529\" ./build-android.sh" >&2
  exit 1
}

ensure_ndk_configured

if [[ $# -eq 0 ]]; then
  ./gradlew assembleDebug
elif [[ "$1" == "debug" ]]; then
  shift || true
  ./gradlew assembleDebug "$@"
elif [[ "$1" == "release" ]]; then
  shift || true
  ./gradlew assembleRelease "$@"
elif [[ "$1" == "clean" ]]; then
  shift || true
  ./gradlew clean "$@"
elif [[ "$1" == "install" ]]; then
  shift || true
  ./gradlew installDebug "$@"
else
  ./gradlew "$@"
fi

echo ""
if [[ -d app/build/outputs/apk ]]; then
  echo "APK files:"
  find app/build/outputs/apk -name '*.apk' 2>/dev/null | sort || true
fi
