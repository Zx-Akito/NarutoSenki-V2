package re.naruto.game;

import android.content.Context;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.WindowManager;

import org.cocos2dx.lib.Cocos2dxActivity;
import org.cocos2dx.lib.Cocos2dxGLSurfaceView;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;

import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.Response;
import okhttp3.WebSocket;
import okhttp3.WebSocketListener;
import okio.ByteString;

public class NarutoSenki extends Cocos2dxActivity{
	private static final String TAG = "NarutoWs";
	private static final Object WS_LOCK = new Object();
	private static final ExecutorService WS_EXECUTOR = Executors.newSingleThreadExecutor(new ThreadFactory() {
		@Override
		public Thread newThread(Runnable r) {
			Thread t = new Thread(r, "NarutoWs");
			t.setDaemon(true);
			return t;
		}
	});
	private static final OkHttpClient WS_CLIENT = new OkHttpClient.Builder()
			.readTimeout(0, TimeUnit.MILLISECONDS)
			.connectTimeout(30, TimeUnit.SECONDS)
			.writeTimeout(30, TimeUnit.SECONDS)
			.callTimeout(0, TimeUnit.MILLISECONDS)
			.retryOnConnectionFailure(true)
			.build();
	private static final String EVENT_OPEN = "open";
	private static final String EVENT_MESSAGE = "message";
	private static final String EVENT_ERROR = "error";
	private static final String EVENT_CLOSE = "close";

	private static volatile WebSocket sWebSocket = null;
	private static volatile boolean sIsConnected = false;
	private static NarutoSenki sInstance = null;

	private static native void nativeWsJniBoot();

	protected void onCreate(Bundle savedInstanceState){
		super.onCreate(savedInstanceState);
		nativeWsJniBoot();
		sInstance = this;
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON, WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
	}

	public Cocos2dxGLSurfaceView onCreateGLSurfaceView() {
		return new LuaGLSurfaceView(this);
	}

	@Override
	protected void onDestroy() {
		wsDisconnect();
		super.onDestroy();
	}

	public static boolean wsConnect(final String url) {
		final String trimmed = url != null ? url.trim() : "";
		if (trimmed.isEmpty()) {
			Log.w(TAG, "wsConnect: empty url");
			return false;
		}

		WS_EXECUTOR.execute(new Runnable() {
			@Override
			public void run() {
				synchronized (WS_LOCK) {
					if (sWebSocket != null) {
						sWebSocket.cancel();
						sWebSocket = null;
					}
					sIsConnected = false;

					try {
						Request request = new Request.Builder().url(trimmed).build();
						sWebSocket = WS_CLIENT.newWebSocket(request, new WebSocketListener() {
							@Override
							public void onOpen(WebSocket webSocket, Response response) {
								sIsConnected = true;
								dispatchWebSocketEvent(EVENT_OPEN, "");
							}

							@Override
							public void onMessage(WebSocket webSocket, String text) {
								dispatchWebSocketEvent(EVENT_MESSAGE, text != null ? text : "");
							}

							@Override
							public void onMessage(WebSocket webSocket, ByteString bytes) {
								dispatchWebSocketEvent(EVENT_MESSAGE, bytes != null ? bytes.hex() : "");
							}

							@Override
							public void onFailure(WebSocket webSocket, Throwable t, Response response) {
								sIsConnected = false;
								synchronized (WS_LOCK) {
									if (sWebSocket == webSocket) {
										sWebSocket = null;
									}
								}
								final String summary = formatWsFailureMessage(t, response);
								Log.e(TAG, "onFailure url=" + trimmed, t);
								dispatchWebSocketEvent(EVENT_ERROR, summary);
							}

							@Override
							public void onClosed(WebSocket webSocket, int code, String reason) {
								sIsConnected = false;
								synchronized (WS_LOCK) {
									if (sWebSocket == webSocket) {
										sWebSocket = null;
									}
								}
								dispatchWebSocketEvent(EVENT_CLOSE, reason != null ? reason : "");
							}
						});
					} catch (IllegalArgumentException e) {
						sWebSocket = null;
						sIsConnected = false;
						Log.e(TAG, "wsConnect: bad url string", e);
						dispatchWebSocketEvent(EVENT_ERROR,
								e.getMessage() != null ? e.getMessage() : e.toString());
					} catch (Exception e) {
						sWebSocket = null;
						sIsConnected = false;
						Log.e(TAG, "wsConnect: failed to create WebSocket url=" + trimmed, e);
						dispatchWebSocketEvent(EVENT_ERROR,
								e.getMessage() != null ? e.getMessage() : e.toString());
					}
				}
			}
		});
		return true;
	}

	private static String formatWsFailureMessage(Throwable t, Response response) {
		String base = "unknown websocket error";
		if (t != null) {
			base = t.getMessage();
			if (base == null || base.trim().isEmpty()) {
				base = t.toString();
			}
		}
		if (response == null) {
			return base;
		}
		StringBuilder sb = new StringBuilder(base);
		sb.append(" (HTTP ").append(response.code()).append(")");
		if (response.message() != null && !response.message().isEmpty()) {
			sb.append(": ").append(response.message());
		}
		return sb.toString();
	}

	public static void wsSend(final String message) {
		WebSocket webSocket = sWebSocket;
		if (webSocket == null || !sIsConnected) {
			return;
		}
		webSocket.send(message != null ? message : "");
	}

	public static void wsDisconnect() {
		synchronized (WS_LOCK) {
			WebSocket webSocket = sWebSocket;
			sWebSocket = null;
			sIsConnected = false;
			if (webSocket != null) {
				webSocket.close(1000, "client_disconnect");
			}
		}
	}

	public static boolean wsIsConnected() {
		return sIsConnected;
	}

	private static void dispatchWebSocketEvent(final String eventName, final String payload) {
		final NarutoSenki activity = sInstance;
		if (activity == null) {
			nativeOnWebSocketEvent(eventName, payload);
			return;
		}

		activity.runOnGLThread(new Runnable() {
			@Override
			public void run() {
				nativeOnWebSocketEvent(eventName, payload);
			}
		});
	}

	private static native void nativeOnWebSocketEvent(String eventName, String payload);

     static {
         System.loadLibrary("cocos2dcpp");
     }
}

class LuaGLSurfaceView extends Cocos2dxGLSurfaceView{

	public LuaGLSurfaceView(Context context){
		super(context);
	}

	public boolean onKeyDown(int keyCode, KeyEvent event) {
		// exit program when key back is entered
		if (keyCode == KeyEvent.KEYCODE_BACK) {
			android.os.Process.killProcess(android.os.Process.myPid());
		}
		return super.onKeyDown(keyCode, event);
	}
}
