--
-- CreditLayer
--
function CreditsLayer_BackToStartMenu()
    audio.stopMusic(true)
    audio.playSound('Audio/Menu/cancel.wav')
    local menuScene = CCScene:create()
    local menuLayer = StartMenu:create()

    hook.registerInitHandlerOnly(menuLayer)
    menuScene:addChild(menuLayer)
    director.replaceSceneWithFade(menuScene, 1)
end
