--
-- SelectButton
--
local unsupportedList = {
    -- Here are not supported or in dev characters
}

-- init values to lower case
for k, v in pairs(unsupportedList) do unsupportedList[k] = string.lower(v) end

function ns.addUnsupportCharacter(name)
    if name ~= nil then
        if table.has(name) then
            log('Add error %s already in unsupported table', name)
        else
            unsupportedList[#unsupportedList + 1] = name
        end
    end
end

SelectButton = class('SelectButton', function() return Button:create() end)

SelectButton.Char_None = 'None'
SelectButton.Char_None2 = 'None2'
SelectButton.Type = {
    Menu = 0,
    Unlock1 = 1,
    Unlock2 = 2,
    skill1 = 3,
    skill2 = 4,
    skill3 = 5,
    Ougis1 = 6,
    Ougis2 = 7
}

function SelectButton:ctor()
    self:setPressedActionEnabled(false)

    local function touchEvent(sender, event)
        if event == ns.TouchEventType.began then
            -- button:setTitleText('Touch Down')
            self:click()
        elseif event == ns.TouchEventType.moved then
            -- button:setTitleText('Touch Move')
        elseif event == ns.TouchEventType.ended then
            -- button:setTitleText('Touch Up')
        elseif event == ns.TouchEventType.canceled then
            -- button:setTitleText('Touch Cancelled')
        end
    end
    self:addTouchEventListener(touchEvent)

    self._btnType = SelectButton.Type.Menu
    self._isCanBuy = false
    self._isCanBuy2 = false
    self._isUnlock = false
    self._clickTime = 0
    self._isAviable = true
    self._selectLayer = nil
    self._skillLayer = nil
    self._charName = nil
end

function SelectButton:create(image)
    local button = SelectButton.new()
    button:loadTextures(image, image, nil, UI_TEX_TYPE_PLIST)
    button:setAnchorPoint(CCPoint(0, 0))
    return button
end

function SelectButton:click()
    if self._btnType ~= SelectButton.Type.Menu then
        self._skillLayer:setSkillExplain(self._btnType)
        return
    end

    if self._charName == SelectButton.Char_None2 then
        -- tools.setTip('LimitedChar')
        local tip = CCTips:create('LimitedChar')
        self._selectLayer:addChild(tip, 5000)
    elseif self._charName ~= SelectButton.Char_None and self._isAviable and
        self:isSupportedCharacter() then
        audio.stopAllSounds()

        if not self._selectLayer then
            -- audio.playSound(ns.menu.SELECT_SOUND)
            -- self._selectLayer:setSelected(self)
            log('SelectLayer can not be nil')
            return
        end

        if self._selectLayer._playerSelect and
            not self._selectLayer.enableCustomSelect then return end

        if self._selectLayer:noComSelect() then return end

        for _, selectBtn in pairs(self._selectLayer.selectButtons) do
            if selectBtn ~= self then selectBtn._clickTime = 0 end
        end

        if self._clickTime == 0 then
            log('Click character ' .. self._charName)

            if save.isVoice() then
                audio.stopAllSounds()
                audio.playSound(fmt('Audio/Intro/%s.wav', self._charName))
            end
            audio.playSound(ns.menu.SELECT_SOUND)
        elseif self._clickTime == 1 then
            log('Selected character ' .. self._charName)

            audio.playSound('Audio/Menu/confirm2.wav')
            self._isAviable = false
            self:spriteToGrey()
            self:getParent():reorderChild(self, 500)
        end

        self._clickTime = self._clickTime + 1
        self._selectLayer:setSelected(self)
    end
end

function SelectButton:spriteToGrey()
    -- local black = display.newSprite('#Black_select.png')
    local black = ImageView:create()
    black:loadTexture('Black_select.png', UI_TEX_TYPE_PLIST)
    black:setAnchorPoint(CCPoint(0.5, 0.5))
    black:setPosition(CCPoint(17, 32))
    self:addChild(black)
end

function SelectButton:isSupportedCharacter()
    for _, v in ipairs(unsupportedList) do
        if v == string.lower(self._charName) then
            local tip = CCTips:create('LimitedChar')
            self._selectLayer:addChild(tip, 5000)

            log(not self._isAviable and 'Has selected character %s' or
                    'Not support %s ', self._charName)
            return false
        end
    end
    return true
end
