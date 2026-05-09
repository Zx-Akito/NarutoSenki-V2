--[[
    Initial UI
]]

--[[
    Function calls order of `init` operations

    ~~~ sample

    c++: GameScene::init()

    -> local scene = GameScene:create()  [lua]
    ->
    ----- Register callback
    ----- In c++ only called once that will
    ----- set handler to empty after called init fucntion
    -> hook.registerInitHandlerOnly(scene)
    ->
    -> GameScene::init()        [c++]
    -> GameScene::onEnter()     [c++]
    ->
    -> lua_call_init_func       [c++]
    ->
    -> GameScene:init()         [lua]
    ->
    -> unregister lua function handler [c++] [Optional by lua]
    -> ...
]]

require 'ui.GameScene'

require 'ui.SelectButton'

require 'ui.StartMenu'
require 'ui.NetworkLobbyLayer'

require 'ui.CreditLayer'
require 'ui.SelectLayer'
require 'ui.SkillLayer'
