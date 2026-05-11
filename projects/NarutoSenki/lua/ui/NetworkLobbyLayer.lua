--
-- NetworkLobbyLayer
--
NetworkLobbyLayer = class('NetworkLobbyLayer', function() return display.newLayer() end)

function NetworkLobbyLayer:ctor()
    self.statusLabel = nil
    self.messageLabel = nil
    self.isConnected = false
    self.isConnecting = false
    self.isInQueue = false
    self.matchId = nil
    self._queueWaitElapsed = 0
    self:init()
    _G.__networkLobbyLayer = self
end

function NetworkLobbyLayer:create() return NetworkLobbyLayer.new() end

function NetworkLobbyLayer:init()
    tools.addSprites('Select.plist')

    local bgSprite = display.newSprite('blue_bg.png', 0, 0)
    bgSprite:setAnchorPoint(0, 0)
    bgSprite:fullScreen()
    self:addChild(bgSprite, -5)

    -- produce the cloud
    local cloud_left = display.newSprite('#cloud.png', 0, 15)
    cloud_left:setAnchorPoint(0, 0)
    cloud_left:setFlipX(true)
    cloud_left:setFlipY(true)
    self:addChild(cloud_left, 2)

    local cmv1 = CCMoveBy:create(1, CCPoint(-15, 0))
    local cseq1 = CCRepeatForever:create(
                      transition.sequence({cmv1, cmv1:reverse()}))
    cloud_left:runAction(cseq1)

    local cloud_right = display.newSprite('#cloud.png')
    cloud_right:setAnchorPoint(0, 0)
    cloud_right:setPosition(display.width - cloud_right:getContentSize().width,
                            15)
    cloud_right:setFlipY(true)
    self:addChild(cloud_right, 2)

    local cmv2 = CCMoveBy:create(1, CCPoint(15, 0))
    local cseq2 = CCRepeatForever:create(
                      transition.sequence({cmv2, cmv2:reverse()}))
    cloud_right:runAction(cseq2)

    local cloud_top_left = display.newSprite('#cloud.png', 0, 0)
    cloud_top_left:setAnchorPoint(0, 1)
    cloud_top_left:setPosition(0, display.height - 15)
    cloud_top_left:setFlipX(true)
    self:addChild(cloud_top_left, 2)

    local cmv3 = CCMoveBy:create(1, CCPoint(-15, 0))
    local cseq3 = CCRepeatForever:create(
                      transition.sequence({cmv3, cmv3:reverse()}))
    cloud_top_left:runAction(cseq3)

    local cloud_top_right = display.newSprite('#cloud.png')
    cloud_top_right:setAnchorPoint(0, 1)
    cloud_top_right:setPosition(display.width - cloud_top_right:getContentSize().width,
                                display.height - 15)
    self:addChild(cloud_top_right, 2)

    local cmv4 = CCMoveBy:create(1, CCPoint(15, 0))
    local cseq4 = CCRepeatForever:create(
                      transition.sequence({cmv4, cmv4:reverse()}))
    cloud_top_right:runAction(cseq4)

    local menu_bar_b = display.newSprite('menu_bar2.png')
    menu_bar_b:setAnchorPoint(0, 0)
    menu_bar_b:fullScreen()
    self:addChild(menu_bar_b, 2)

    local menu_bar_t = display.newSprite('menu_bar3.png')
    menu_bar_t:setAnchorPoint(0, 0)
    menu_bar_t:setPosition(0, display.height - menu_bar_t:getContentSize().height)
    menu_bar_t:fullScreen()
    self:addChild(menu_bar_t, 2)

    -- Teks dipusatkan: simetris di sekitar tengah layar (vertikal + horizontal).
    self.statusLabel = ui.newTTFLabelWithShadow({
        text = 'Status: Disconnected',
        size = 18,
        x = display.cx,
        y = display.cy + 16,
        align = ui.TEXT_ALIGN_CENTER
    })
    self:addChild(self.statusLabel, 5)

    self.messageLabel = ui.newTTFLabelWithShadow({
        text = 'Click start to find a match',
        size = 14,
        x = display.cx,
        y = display.cy - 16,
        align = ui.TEXT_ALIGN_CENTER
    })
    self:addChild(self.messageLabel, 5)

    self:scheduleUpdateWithPriorityLua(handler(self, NetworkLobbyLayer.update), 0)

    -- Kanan bawah: Return di atas, Start di bawah (sumbu Y ke atas).
    local btnStackX = display.width - 38
    local startBtnY = 52
    local returnBtnY = startBtnY + 64

    local disconnectBtn = ui.newImageMenuItem({
        image = 'UI/return_btn.png',
        listener = handler(self, NetworkLobbyLayer.onDisconnectPressed)
    })
    local disconnectMenu = ui.newMenu({disconnectBtn})
    disconnectMenu:setPosition(btnStackX, returnBtnY)
    self:addChild(disconnectMenu, 5)

    local connectBtn = ui.newImageMenuItem({
        image = '#start_btn.png',
        listener = handler(self, NetworkLobbyLayer.onConnectPressed)
    })
    local connectMenu = ui.newMenu({connectBtn})
    connectMenu:setPosition(btnStackX, startBtnY)
    self:addChild(connectMenu, 5)

end

function NetworkLobbyLayer:setStatus(text)
    local value = 'Status: ' .. tostring(text)
    if self.statusLabel and self.statusLabel.label then
        self.statusLabel.label:setString(value)
    end
    if self.statusLabel and self.statusLabel.shadow1 then
        self.statusLabel.shadow1:setString(value)
    end
end

--- Baris kedua lobby: waktu tunggu (bukan payload JSON mentah dari server).
function NetworkLobbyLayer:setWaitSubtitle(seconds)
    local sec = math.max(0, math.floor(seconds or 0))
    local m = math.floor(sec / 60)
    local s = sec % 60
    local value = string.format('Waiting for enemy: %d:%02d', m, s)
    if self.messageLabel and self.messageLabel.label then
        self.messageLabel.label:setString(value)
    end
    if self.messageLabel and self.messageLabel.shadow1 then
        self.messageLabel.shadow1:setString(value)
    end
end

function NetworkLobbyLayer:onConnectPressed()
    if self.isConnecting then
        self:setStatus('Connecting...')
        return
    end
    if self.isConnected and wsIsConnected() then
        if self.matchId then
            self:setStatus('Match ready: ' .. self.matchId)
            return
        end
        if not self.isInQueue then
            self:setStatus('Queueing...')
            wsSend('{"type":"queue_join"}')
            self.isInQueue = true
        else
            self:setStatus('Queueing...')
        end
        return
    end

    self.isConnecting = true
    self:setStatus('Connecting...')
    local ok = wsConnect('wss://ws.cheetoz.xyz')
    if not ok then
        self.isConnecting = false
        self:setStatus('Connect failed')
    end
end

function NetworkLobbyLayer:onDisconnectPressed()
    if self.isConnected then
        wsSend('{"type":"queue_leave"}')
    end
    wsDisconnect()
    self.isConnecting = false
    self.isConnected = false
    self._queueWaitElapsed = 0
    _G.__networkMatchId = nil
    _G.__networkLobbyLayer = nil
    backToStartMenu()
end

function NetworkLobbyLayer:update(dt)
    if not self.isConnected or not self.isInQueue or self.matchId then
        return
    end
    self._queueWaitElapsed = self._queueWaitElapsed + dt
    self:setWaitSubtitle(self._queueWaitElapsed)
end

local function extractJsonStringField(payload, fieldName)
    local pattern = '"' .. fieldName .. '"%s*:%s*"([^"]+)"'
    return string.match(payload or '', pattern)
end

-- Pola Lua tidak mendukung (true|false); '|' adalah karakter literal.
local function extractJsonBoolField(payload, fieldName)
    local head = '"' .. fieldName .. '"%s*:%s*'
    local v = string.match(payload or '', head .. '(true)')
    if v == 'true' then return true end
    v = string.match(payload or '', head .. '(false)')
    if v == 'false' then return false end
    return nil
end

function NetworkLobbyLayer:handleWebSocketEvent(eventName, payload)
    if eventName == 'open' then
        self.isConnecting = false
        self.isConnected = true
        self.isInQueue = true
        self._queueWaitElapsed = 0
        self:setWaitSubtitle(0)
        self:setStatus('Queueing...')
        wsSend('{"type":"ping"}')
        wsSend('{"type":"queue_join"}')
    elseif eventName == 'message' then
        local messageType = extractJsonStringField(payload, 'type')
        if messageType == 'queue_waiting' then
            self.matchId = nil
            self:setStatus('Queueing...')
        elseif messageType == 'match_found' then
            self.matchId = extractJsonStringField(payload, 'matchId')
            self.isInQueue = false
            _G.__networkMatchId = self.matchId
            _G.__networkVsBot = extractJsonBoolField(payload, 'opponentIsBot') == true
            self:setStatus('Match found: ' .. tostring(self.matchId or '?'))
            _G.__networkLobbyLayer = nil
            enterSelectLayer(GameMode.Classic, false)
        elseif messageType == 'opponent_left' then
            self:setStatus('Opponent left')
            self:setWaitSubtitle(0)
            wsDisconnect()
            self.isConnecting = false
            self.isConnected = false
            self.isInQueue = false
            self.matchId = nil
            _G.__networkMatchId = nil
            _G.__networkLobbyLayer = nil
            backToStartMenu()
        elseif messageType == 'pong' then
            -- keepalive ack
        end
    elseif eventName == 'close' then
        self.isConnecting = false
        self.isConnected = false
        self.isInQueue = false
        self.matchId = nil
        self._queueWaitElapsed = 0
        _G.__networkMatchId = nil
        self:setStatus('Disconnected')
        self:setWaitSubtitle(0)
    elseif eventName == 'error' then
        self.isConnecting = false
        self.isConnected = false
        self.isInQueue = false
        self.matchId = nil
        self._queueWaitElapsed = 0
        _G.__networkMatchId = nil
        local detail = tostring(payload or '')
        if #detail > 120 then
            detail = string.sub(detail, 1, 117) .. '...'
        end
        if detail ~= '' then
            self:setStatus('Error: ' .. detail)
        else
            self:setStatus('Error')
        end
        self:setWaitSubtitle(0)
    end
end
