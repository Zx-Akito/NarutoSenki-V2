--
-- Common Audio Files
--

local musicPerfix   = 'Audio/Music/'
local soundPerfix   = 'Audio/Menu/'

ns.music = {
    Perfix          = musicPerfix,
    INTRO_MUSIC     = musicPerfix .. 'intro_music.m4a',
    LOADING_MUSIC   = musicPerfix .. 'loading_music.m4a',
    SELECT_MUSIC    = musicPerfix .. 'select_music.m4a',
    RANKING_MUSIC   = musicPerfix .. 'ranking_music.m4a',
    MENU_MUSIC      = musicPerfix .. 'menu_music.m4a',
    CREDITS_MUSIC   = musicPerfix .. 'credits_music.m4a',
    BATTLE_MUSIC    = musicPerfix .. 'Battle1.m4a',
}

ns.menu = {
    Perfix          = soundPerfix,
    CONFIRM         = soundPerfix .. 'confirm.wav',
    CREDITS_SOUND   = soundPerfix .. 'credits.wav',
    EXIT_SOUND      = soundPerfix .. 'exit.wav',
    LOGO_CLICK      = soundPerfix .. 'chang_btn.wav',
    MENU_INTRO      = soundPerfix .. 'intro.wav',
    MENU_INTRO2     = soundPerfix .. 'intro2.wav',
    NETWORK_SOUND   = soundPerfix .. 'arcade.wav',
    SELECT_SOUND    = soundPerfix .. 'select.wav',
    TRAINING_SOUND  = soundPerfix .. 'training.wav',
}
