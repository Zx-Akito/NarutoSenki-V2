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
    self.countdownSeconds = nil
    self.countdownElapsed = 0

    self:init()
    _G.__networkLobbyLayer = self
end

function NetworkLobbyLayer:create() return NetworkLobbyLayer.new() end

function NetworkLobbyLayer:startMatchCountdown(seconds)
    self.countdownSeconds = seconds or 10
    self.countdownElapsed = 0
    self:setStatus('Match found: ' .. tostring(self.matchId or '?') ..
                       ' | Start in ' .. tostring(self.countdownSeconds) .. 's')
    self:scheduleUpdateWithPriorityLua(handler(self, self.update), 0)
end

function NetworkLobbyLayer:stopMatchCountdown()
    if self.countdownSeconds ~= nil then self:unscheduleUpdate() end
    self.countdownSeconds = nil
    self.countdownElapsed = 0
end

function NetworkLobbyLayer:update(dt)
    if self.countdownSeconds == nil then return end

    self.countdownElapsed = self.countdownElapsed + dt
    if self.countdownElapsed < 1 then return end

    self.countdownElapsed = self.countdownElapsed - 1
    self.countdownSeconds = self.countdownSeconds - 1

    if self.countdownSeconds <= 0 then
        self:stopMatchCountdown()
        _G.__networkMatchId = self.matchId
        _G.__networkLobbyLayer = nil
        enterSelectLayer(GameMode.Classic, false)
        return
    end

    self:setStatus('Match found: ' .. tostring(self.matchId or '?') ..
                       ' | Start in ' .. tostring(self.countdownSeconds) .. 's')
end

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

    local title = ui.newTTFLabelWithShadow({
        text = 'Network Lobby',
        size = 24,
        x = display.cx,
        y = display.height - 80,
        align = ui.TEXT_ALIGN_CENTER
    })
    self:addChild(title, 5)

    self.statusLabel = ui.newTTFLabelWithShadow({
        text = 'Status: Disconnected',
        size = 18,
        x = display.cx,
        y = display.cy + 30,
        align = ui.TEXT_ALIGN_CENTER
    })
    self:addChild(self.statusLabel, 5)

    self.messageLabel = ui.newTTFLabelWithShadow({
        text = 'Server: -',
        size = 14,
        x = display.cx,
        y = display.cy - 2,
        align = ui.TEXT_ALIGN_CENTER
    })
    self:addChild(self.messageLabel, 5)

    local connectBtn = ui.newImageMenuItem({
        image = '#start_btn.png',
        listener = handler(self, NetworkLobbyLayer.onConnectPressed)
    })
    local connectMenu = ui.newMenu({connectBtn})
    connectMenu:setPosition(display.cx, 55)
    self:addChild(connectMenu, 5)

    local disconnectBtn = ui.newImageMenuItem({
        image = 'UI/return_btn.png',
        listener = handler(self, NetworkLobbyLayer.onDisconnectPressed)
    })
    local disconnectMenu = ui.newMenu({disconnectBtn})
    disconnectMenu:setPosition(display.width - 38, 55)
    self:addChild(disconnectMenu, 5)

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

function NetworkLobbyLayer:setMessage(text)
    local raw = tostring(text or '-')
    local maxLen = 72
    if string.len(raw) > maxLen then
        raw = string.sub(raw, 1, maxLen) .. '...'
    end
    local value = 'Server: ' .. raw
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
    local ok = wsConnect('ws://127.0.0.1:8080')
    if not ok then
        self.isConnecting = false
        self:setStatus('Connect failed')
    end
end

function NetworkLobbyLayer:onDisconnectPressed()
    self:stopMatchCountdown()
    if self.isConnected then
        wsSend('{"type":"queue_leave"}')
    end
    wsDisconnect()
    self.isConnecting = false
    self.isConnected = false
    _G.__networkMatchId = nil
    _G.__networkLobbyLayer = nil
    backToStartMenu()
end

local function extractJsonStringField(payload, fieldName)
    local pattern = '"' .. fieldName .. '"%s*:%s*"([^"]+)"'
    return string.match(payload or '', pattern)
end

function NetworkLobbyLayer:handleWebSocketEvent(eventName, payload)
    if eventName == 'open' then
        self.isConnecting = false
        self.isConnected = true
        self.isInQueue = true
        self:setStatus('Queueing...')
        wsSend('{"type":"ping"}')
        wsSend('{"type":"queue_join"}')
    elseif eventName == 'message' then
        self:setMessage(payload or '-')
        local messageType = extractJsonStringField(payload, 'type')
        if messageType == 'queue_waiting' then
            self:stopMatchCountdown()
            self.matchId = nil
            self:setStatus('Queueing...')
        elseif messageType == 'match_found' then
            self.matchId = extractJsonStringField(payload, 'matchId')
            self.isInQueue = false
            _G.__networkMatchId = self.matchId
            self:startMatchCountdown(10)
        elseif messageType == 'opponent_left' then
            self:stopMatchCountdown()
            self:setStatus('Opponent left')
            self:setMessage('Match cancelled')
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
        self:stopMatchCountdown()
        self.isConnecting = false
        self.isConnected = false
        self.isInQueue = false
        self.matchId = nil
        _G.__networkMatchId = nil
        self:setStatus('Disconnected')
    elseif eventName == 'error' then
        self:stopMatchCountdown()
        self.isConnecting = false
        self.isConnected = false
        self.isInQueue = false
        self.matchId = nil
        _G.__networkMatchId = nil
        self:setStatus('Error')
        self:setMessage(payload or '-')
    end
end
