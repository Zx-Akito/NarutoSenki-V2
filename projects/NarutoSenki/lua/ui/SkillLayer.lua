--
-- SelectLayer
--
local transformList = {
    ['Naruto'] = 'SageNaruto',
    ['SageNaruto'] = 'RikudoNaruto',
    ['Jiraiya'] = 'SageJiraiya',
    ['Sasuke'] = 'ImmortalSasuke',
    ['Pain'] = 'Nagato'
}

local useFull2ImageList = {
    'Hinata', 'Sakura', 'Ino', 'Tsunade', 'Tenten', 'Konan', 'Karin'
}

SkillLayer = class('SkillLayer', function() return display.newLayer() end)

function SkillLayer:ctor(selectLayer)
    self._selectLayer = selectLayer
    self._skillExplain = nil

    self._buyLayer = nil
    self._buyType = SelectButton.Type.Menu
    self._fengSprite = nil
    self._fengSprite2 = nil
    self._autoMove = true

    self.coinLabel = nil
    self._heroFull = nil
    self.changemenu = nil
    self.skillGroup = nil
    self.bgSprite = nil
    self.bonds = nil

    self:init()
    self:initInterface()
end

function SkillLayer:create(selectLayer) return SkillLayer.new(selectLayer) end

function SkillLayer:init()
    log('Initial SkillLayer')

    -- produce groud
    local gold_left = display.newSprite('#gold_left.png')
    gold_left:setAnchorPoint(0, 0)
    gold_left:setPosition(0, 20)
    self:addChild(gold_left, 1)

    local gold_right = display.newSprite('#gold_right.png')
    gold_right:setAnchorPoint(0, 1)
    gold_right:setPosition(display.width - gold_right:getContentSize().width -
                               20, display.height - 20)
    self:addChild(gold_right, 1)

    -- produce the cloud
    local cloud_left = display.newSprite('#cloud.png')
    cloud_left:setPosition(0, 15)
    cloud_left:setFlipX(true)
    cloud_left:setFlipY(true)
    cloud_left:setAnchorPoint(0, 0)
    self:addChild(cloud_left, 2)

    local cmv1 = CCMoveBy:create(1, CCPoint(-15, 0))
    local cseq1 = CCRepeatForever:create(
                      transition.sequence({cmv1, cmv1:reverse()}))
    cloud_left:runAction(cseq1)

    local cloud_right = display.newSprite('#cloud.png')
    cloud_right:setPosition(display.width - cloud_right:getContentSize().width,
                            15)
    cloud_right:setFlipY(true)
    cloud_right:setAnchorPoint(0, 0)
    self:addChild(cloud_right, 2)

    local cmv2 = CCMoveBy:create(1, CCPoint(15, 0))
    local cseq2 = CCRepeatForever:create(
                      transition.sequence({cmv2, cmv2:reverse()}))
    cloud_right:runAction(cseq2)

    local cloud_top = display.newSprite('#cloud_top.png')
    cloud_top:setPosition(display.cx, display.height -
                              (cloud_top:getContentSize().height + 12))
    cloud_top:setAnchorPoint(0.5, 0)
    self:addChild(cloud_top, 1)

    -- produce the menu_bar
    local menu_bar_b = display.newSprite('menu_bar2.png')
    menu_bar_b:setAnchorPoint(0, 0)
    menu_bar_b:fullScreen()
    self:addChild(menu_bar_b, 2)

    local menu_bar_t = display.newSprite('menu_bar3.png')
    menu_bar_t:setAnchorPoint(0, 0)
    menu_bar_t:setPosition(0,
                           display.height - menu_bar_t:getContentSize().height)
    menu_bar_t:fullScreen()
    self:addChild(menu_bar_t, 2)

    local record_title = display.newSprite('#record_title.png')
    record_title:setAnchorPoint(0, 0)
    record_title:setPosition(2, display.height -
                                 record_title:getContentSize().height - 2)
    self:addChild(record_title, 3)

    local start_bt = ui.newImageMenuItem({
        image = display.newSprite('#return_btn.png'),
        listener = handler(self, SkillLayer.onCancel)
    })
    local menu = ui.newMenu({start_bt})
    menu:setPosition(display.width - 38, 86)
    self:addChild(menu, 5)

    return true
end

function SkillLayer:initInterface()
    if self._selectLayer then self.selectHero = self._selectLayer.selectHero end

    local bgSprite = display.newSprite('blue_bg.png', 0, 0)
    bgSprite:fullScreen()
    bgSprite:setAnchorPoint(0, 0)
    self:addChild(bgSprite, -5)

    local winNum = tools.readWinNumFromSQL(self.selectHero)
    local loseNum = tools.readLoseNumFromSQL(self.selectHero)

    local rank_src
    local isBlink = false
    local skillbg_src
    if winNum >= 300 then
        skillbg_src = 'skill_bg6.png'
        rank_src = 'rank_sss.png'
        isBlink = true
    elseif winNum >= 200 then
        skillbg_src = 'skill_bg5.png'
        rank_src = 'rank_ss.png'
        isBlink = true
    elseif winNum >= 100 then
        skillbg_src = 'skill_bg4.png'
        rank_src = 'rank_s.png'
        isBlink = true
    elseif winNum >= 50 then
        skillbg_src = 'skill_bg3.png'
        rank_src = 'rank_a.png'
    elseif winNum >= 25 then
        skillbg_src = 'skill_bg2.png'
        rank_src = 'rank_b.png'
    else
        skillbg_src = 'skill_bg1.png'
        rank_src = 'rank_c.png'
    end

    local skill_bg = display.newSprite('#' .. skillbg_src, display.cx,
                                       display.cy)
    self:addChild(skill_bg, 2)
    self.bgSprite = skill_bg

    self:trySetCharacterChangeButton(self.selectHero)

    local goldBG = display.newSprite('#gold_bg.png')
    goldBG:setAnchorPoint(0.5, 0)
    goldBG:setPosition(display.cx + skill_bg:getContentSize().width / 4 + 25,
                       display.cy + skill_bg:getContentSize().width / 2 - 74)
    self:addChild(goldBG, 9)

    local coinsNum = tools.readCoinFromSQL()
    self.coinLabel = CCLabelBMFont:create(tools.formatCoinDisplay(coinsNum), 'Fonts/1.fnt')
    self.coinLabel:setScale(0.3)
    self.coinLabel:setAnchorPoint(0.5, 0)
    self.coinLabel:setPosition(goldBG:getPositionX() -
                                   goldBG:getContentSize().width / 2 + 40,
                               goldBG:getPositionY() + 4)
    self:addChild(self.coinLabel, 15)

    local bondBG = display.newSprite('#golds_1.png')
    bondBG:setAnchorPoint(0.5, 0)
    bondBG:setPosition(
        goldBG:getPositionX() - goldBG:getContentSize().width / 2 + 15,
        goldBG:getPositionY() + 4)
    self:addChild(bondBG, 15)

    local bondBG2 = display.newSprite('#golds_2.png')
    local bonds = display.newProgressTimer(bondBG2, display.PROGRESS_TIMER_BAR)
    self.bonds = bonds
    bonds:setAnchorPoint(0.5, 0)
    -- bonds:setMidpoint(CCPoint(0, 0)) -- is default value
    bonds:setBarChangeRate(CCPoint(0, 1))

    if coinsNum >= 99999 then
        bonds:setPercentage(100)
    elseif coinsNum >= 80000 then
        bonds:setPercentage(coinsNum / 1000)
    end

    bonds:setPosition(
        goldBG:getPositionX() - goldBG:getContentSize().width / 2 + 15,
        goldBG:getPositionY() + 4)
    self:addChild(bonds, 20)

    local imgPath
    if table.has(useFull2ImageList, self.selectHero) and winNum >= 100 then
        imgPath = '#' .. self.selectHero .. '_full2.png'
    else
        imgPath = '#' .. self.selectHero .. '_full.png'
    end
    self._heroFull = display.newSprite(imgPath)
    self._heroFull:setAnchorPoint(0.5, 0)
    self._heroFull:setPosition(display.cx, display.cy -
                                   skill_bg:getContentSize().height / 2 + 23)
    self:addChild(self._heroFull, 5)

    if isBlink then
        local blink = display.newSprite('#skill_blink6.png', display.cx,
                                        display.cy)
        self:addChild(blink, 9)

        local tempArray = CCArray:create()

        for i = 1, 6 do
            local frame = display.newSpriteFrame('skill_blink' .. i .. '.png')
            tempArray:addObject(frame)
        end

        local tempAnimation = CCAnimation:createWithSpriteFrames(tempArray, 0.1)

        blink:runAction(CCRepeatForever:create(
                            transition.sequence({
                CCAnimate:create(tempAnimation), CCDelayTime:create(2.0)
            })))
    end

    local frameSprite = display.newSprite('#skill_frame.png', display.cx,
                                          display.cy)
    self:addChild(frameSprite, 10)

    local rankSprite = display.newSprite('#' .. rank_src)
    rankSprite:setAnchorPoint(0, 0)
    rankSprite:setPosition(skill_bg:getPositionX() -
                               skill_bg:getContentSize().width / 2 + 5,
                           skill_bg:getPositionY() +
                               skill_bg:getContentSize().height / 2 - 20)
    self:addChild(rankSprite, 11)

    local detailBG1 = display.newSprite('#detail_bg.png')
    detailBG1:setAnchorPoint(0, 0)
    detailBG1:setPosition(skill_bg:getPositionX() -
                              skill_bg:getContentSize().width / 2 + 8,
                          skill_bg:getPositionY() -
                              skill_bg:getContentSize().height / 2 + 74)
    self:addChild(detailBG1, 5)

    local recordTime = tools.readRecordTimeFromSQL(self.selectHero)
    local bestLabel
    if recordTime == '' then
        bestLabel = CCLabelBMFont:create('00:00:00', 'Fonts/1.fnt')
    else
        bestLabel = CCLabelBMFont:create(recordTime, 'Fonts/1.fnt')
    end

    bestLabel:setScale(0.3)
    bestLabel:setAnchorPoint(0, 0)
    bestLabel:setPosition(detailBG1:getPositionX() + 8,
                          detailBG1:getPositionY() + 7)
    self:addChild(bestLabel, 6)

    local detailBG2 = display.newSprite('#detail_bg.png')
    detailBG2:setAnchorPoint(0, 0)
    detailBG2:setPosition(skill_bg:getPositionX() +
                              skill_bg:getContentSize().width / 2 -
                              detailBG2:getContentSize().width - 8,
                          skill_bg:getPositionY() -
                              skill_bg:getContentSize().height / 2 + 74)
    self:addChild(detailBG2, 5)

    local recordLabel = CCLabelBMFont:create(
        string.format('%dW/%dL', winNum, loseNum), 'Fonts/1.fnt')
    recordLabel:setScale(0.3)
    recordLabel:setAnchorPoint(0, 0)
    recordLabel:setPosition(detailBG2:getPositionX() + 12,
                            detailBG2:getPositionY() + 7)
    self:addChild(recordLabel, 6)

    self:updateSkillGroup()
    self:scheduleUpdateWithPriorityLua(handler(self, SkillLayer.update), 0)
end

function SkillLayer:onChangeBtn()
    audio.playSound('Audio/Menu/chang_btn.wav')
    self.skillGroup:removeFromParent()
    self.changemenu:removeFromParent()

    local selectedHeroName = transformList[self.selectHero]
    if selectedHeroName then self.selectHero = selectedHeroName end
    self:trySetCharacterChangeButton(selectedHeroName, 2)

    self._heroFull:setDisplayFrame(display.newSpriteFrame(self.selectHero ..
                                                              '_full.png'))
    self:updateSkillGroup()
end

function SkillLayer:updateSkillGroup()
    self.skillGroup = TouchGroup:create()

    for i = 1, 5 do
        local skill_btn = SelectButton:create(
                              self.selectHero .. '_skill' .. i .. '.png')

        if i == 4 then
            skill_btn._isUnlock = true
        elseif i == 5 then
            skill_btn._isCanBuy2 = false
            skill_btn._isUnlock = true
        end

        skill_btn:setPosition(CCPoint((i - 1) * 46, 0))
        skill_btn._btnType = i + 2

        skill_btn._selectLayer = self._selectLayer
        skill_btn._skillLayer = self
        self.skillGroup:addWidget(skill_btn)
    end

    self.skillGroup:setPosition(self.bgSprite:getPositionX() -
                                    self.bgSprite:getContentSize().width / 2 + 2,
                                60)
    self:addChild(self.skillGroup, 500)
end

function SkillLayer:setSkillExplain(buttonType)
    audio.playSound('Audio/Menu/select.wav')
    if self._skillExplain then self._skillExplain:removeFromParent() end

    local imgPath
    if buttonType == SelectButton.Type.Unlock1 then
        imgPath = 'fenglabel.png'
    elseif buttonType == SelectButton.Type.Unlock2 then
        imgPath = 'fenglabel2.png'
    else
        imgPath = self.selectHero .. '_label' .. (buttonType - 2) .. '.png'
    end

    local clipper = CCClippingNode:create()
    local stencil = display.newSprite('#clipperMask.png')
    stencil:setAnchorPoint(0, 0)
    clipper:setStencil(stencil)

    self._skillExplain = display.newSprite('#' .. imgPath)
    self._skillExplain:setAnchorPoint(0, 0)
    self._skillExplain:setPositionX(10)

    clipper:setPosition(display.cx - self.bgSprite:getContentSize().width / 2 +
                            2, display.cy -
                            self.bgSprite:getContentSize().height / 2 + 4)
    clipper:addChild(self._skillExplain)

    self:addChild(clipper, 600)
end

function SkillLayer:update(dt)
    if not self._skillExplain or not self._autoMove then return end

    local lableX = self._skillExplain:getContentSize().width

    if self._skillExplain:getPositionX() >= -lableX then
        self._skillExplain:setPositionX(self._skillExplain:getPositionX() - 0.6)
    else
        self._skillExplain:setPositionX(320)
    end
end

function SkillLayer:onCancel()
    self._skillExplain = nil
    self._buyLayer = nil
    self._fengSprite = nil
    self._fengSprite2 = nil

    self:removeFromParent()
    audio.playSound('Audio/Menu/cancel.wav')

    director.popScene()
end

function SkillLayer:trySetCharacterChangeButton(name, level)
    if not level then level = 1 end
    if not name or level < 1 then return end

    if transformList[name] ~= nil then
        local index = level == 1 and '' or level

        local change_bt = ui.newImageMenuItem({
            image = display.newSprite('#change_btn' .. index .. '.png'),
            listener = handler(self, SkillLayer.onChangeBtn)
        })
        self.changemenu = ui.newMenu({change_bt})
        self.changemenu:setPosition(display.cx, display.cy -
                                        self.bgSprite:getContentSize().height /
                                        2 + 120)
        local fd = CCFadeOut:create(1.0)
        local seq = CCRepeatForever:create(
                        transition.sequence({fd, fd:reverse()}))
        change_bt:runAction(seq)
        self:addChild(self.changemenu, 15)
    end
end
