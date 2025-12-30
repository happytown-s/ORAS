/*
  ==============================================================================

    LooperTrackUi.cpp
    Created: 21 Sep 2025 5:33:29pm
    Author:  mt sh

  ==============================================================================
*/

#include "LooperTrackUi.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include "PizzaColours.h"

//==============================================================================
void LooperTrackUi::paint(juce::Graphics& g)
{
	auto bounds = getLocalBounds().toFloat();
	
	// レイアウト定義
	float width = bounds.getWidth();
	float buttonSize = width; // 正方形
	
	juce::Rectangle<float> buttonArea(0, 0, buttonSize, buttonSize);
	// 下部に15pxの隙間を追加し、さらにボタンとの間にも10pxの隙間(gap)を作る
	float gap = 10.0f;
	juce::Rectangle<float> bottomArea(0, buttonSize + gap, width, bounds.getHeight() - buttonSize - 15.0f - gap); 

	// --- 1. Top Selection Button ---
	// 影と角丸
	g.setColour(PizzaColours::DeepOvenBrown.withAlpha(0.2f));
	g.fillRoundedRectangle(buttonArea.translated(2, 2), 8.0f);

	if(isSelected){
		g.setColour(PizzaColours::TomatoRed);
	}else if(isMouseOver && buttonArea.contains(getMouseXYRelative().toFloat())){
		g.setColour(PizzaColours::BasilGreen.withAlpha(0.6f));
	}else{
		g.setColour(PizzaColours::CreamDough.darker(0.1f));
	}
	g.fillRoundedRectangle(buttonArea, 8.0f);
    
    // 枠線
	g.setColour(PizzaColours::DeepOvenBrown);
	g.drawRoundedRectangle(buttonArea, 8.0f, 2.0f);

	// 状態表示 (光るボーダー)
	if(state == TrackState::Recording){
		g.setColour(PizzaColours::TomatoRed.darker(0.3f));
		g.fillRoundedRectangle(buttonArea, 8.0f);
		drawGlowingBorder(g, juce::Colours::red, buttonArea);
	}else if(state == TrackState::Playing){
		g.setColour(PizzaColours::BasilGreen);
		g.fillRoundedRectangle(buttonArea, 8.0f);
		drawGlowingBorder(g, juce::Colours::lightgreen, buttonArea);
	} else if (state == TrackState::Standby) {
        g.setColour(PizzaColours::CheeseYellow);
        g.fillRoundedRectangle(buttonArea, 8.0f);
        drawGlowingBorder(g, juce::Colours::yellow, buttonArea);
    }

	// トラック番号
	g.setColour(PizzaColours::DeepOvenBrown);
	g.setFont(20.0f);
	g.drawText(juce::String(trackId), buttonArea, juce::Justification::centred, true);


	// --- 2. Level Meter (Bottom Left) ---
	// メーターエリア定義 (スライダーの左側)
	juce::Rectangle<float> meterArea = bottomArea.removeFromLeft(width * 0.4f).reduced(4.0f, 0.0f); // 左右のみreduce
	
	// 背景
	g.setColour(juce::Colours::black.withAlpha(0.2f));
	g.fillRoundedRectangle(meterArea, 3.0f);
	
	// レベルバー
	if (currentRmsLevel > 0.0f)
	{
		float levelHeight = meterArea.getHeight() * juce::jmin(currentRmsLevel * 4.0f, 1.0f); // ゲイン少し上げて表示
		juce::Rectangle<float> levelRect(meterArea.getX(), 
										 meterArea.getBottom() - levelHeight, 
										 meterArea.getWidth(), 
										 levelHeight);
		
		g.setColour(PizzaColours::BasilGreen);
		if (currentRmsLevel > 0.8f) g.setColour(PizzaColours::TomatoRed); // クリップ付近
		
		g.fillRoundedRectangle(levelRect, 3.0f);
	}
}

void LooperTrackUi::drawGlowingBorder(juce::Graphics& g, juce::Colour glowColour, juce::Rectangle<float> area)
{
	float totalPerimeter = area.getWidth() * 2 + area.getHeight()* 2;
	float drawLength = flashProgress * totalPerimeter;

	juce::Line<float> lines[4] = {
		{area.getTopLeft(), area.getTopRight()},
		{area.getTopRight(), area.getBottomRight()},
		{area.getBottomRight(), area.getBottomLeft()},
		{area.getBottomLeft(), area.getTopLeft()}
	};
	g.setColour(glowColour.brighter(0.5f));

	float remaining = drawLength;

	for(int i = 0; i < 4; ++i){
		auto lineLength = lines[i].getLength();
		if(remaining <= 0) break;

		if(remaining < lineLength){
			juce::Point<float> end = lines[i].getStart() + (lines[i].getEnd() - lines[i].getStart()) * (remaining / lineLength);
			g.drawLine(lines[i].getStart().x,lines[i].getStart().y,end.x,end.y, 4.0f);
			break;
		}
		else{
			g.drawLine(lines[i], 4.0f);
			remaining -= lineLength;
		}
	}
}

//==============================================================================
void LooperTrackUi::FaderLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                                       float sliderPos, float minSliderPos, float maxSliderPos,
                                                       const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    // メモリ（目盛り）の描画
    g.setColour(PizzaColours::DeepOvenBrown.withAlpha(0.3f));
    int numTicks = 5; // 0, 25, 50, 75, 100%
    float trackSpecWidth = 4.0f;
    float centerX = x + width * 0.5f;
    
    for (int i = 0; i < numTicks; ++i)
    {
        float ratio = (float)i / (float)(numTicks - 1); // 0.0 to 1.0
        float tickY = y + height - (height * ratio); // 下から上へ
        
        // トラックの両側に短い線を描く
        float tickSize = (i == 0 || i == numTicks - 1 || i == 2) ? 6.0f : 3.0f; // 両端と真ん中は長く
        
        g.drawLine(centerX - trackSpecWidth - tickSize, tickY, centerX - trackSpecWidth + 2.0f, tickY, 1.0f); // 左側
        g.drawLine(centerX + trackSpecWidth - 2.0f, tickY, centerX + trackSpecWidth + tickSize, tickY, 1.0f); // 右側
    }

    // トラック（溝）の描画
    g.setColour(PizzaColours::DeepOvenBrown.withAlpha(0.2f));
    juce::Rectangle<float> track(centerX - trackSpecWidth * 0.5f, 
                                 (float)y, 
                                 trackSpecWidth, 
                                 (float)height);
    g.fillRoundedRectangle(track, 2.0f);

    // つまみ（Thumb）の描画 - 横長にする
    // sliderPos は中心位置
    float thumbWidth = (float)width * 0.8f; // 幅の80%を使う
    float thumbHeight = 12.0f;
    
    juce::Rectangle<float> thumb(0, 0, thumbWidth, thumbHeight);
    thumb.setCentre(centerX, sliderPos);
    
    // つまみの色
    g.setColour(PizzaColours::TomatoRed);
    g.fillRoundedRectangle(thumb, 4.0f);
    
    // つまみの装飾（中央線）
    g.setColour(PizzaColours::CreamDough.withAlpha(0.8f));
    g.drawLine(thumb.getX() + 2.0f, thumb.getCentreY(), thumb.getRight() - 2.0f, thumb.getCentreY(), 2.0f);
}

//==============================================================================
LooperTrackUi::LooperTrackUi(int id, TrackState initState)
	: trackId(id), state(initState)
{
	// スライダー設定
	gainSlider.setSliderStyle(juce::Slider::LinearVertical);
	gainSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	gainSlider.setRange(0.0, 1.0); // 0倍〜1倍
	gainSlider.setValue(0.75);     // デフォルトを75%（メモリ位置）に設定
    
    // カスタムLookAndFeelを適用
    gainSlider.setLookAndFeel(&faderLookAndFeel);

	gainSlider.onValueChange = [this]()
	{
		if(onGainChange)
			onGainChange((float)gainSlider.getValue());
	};

	addAndMakeVisible(gainSlider);
}

void LooperTrackUi::resized()
{
	auto bounds = getLocalBounds();
	float width = bounds.getWidth();
	float buttonSize = width;
	
	// フェーダーエリア (メーターの右側)
	// 下部に15pxの隙間、上部(ボタン下)に10pxの隙間を作る
	int gap = 10;
	juce::Rectangle<int> bottomArea(0, (int)buttonSize + gap, width, bounds.getHeight() - (int)buttonSize - 15 - gap);
	
	// 左40%はメーター用に空けて、右60%にスライダーを配置
	gainSlider.setBounds(bottomArea.removeFromRight((int)(width * 0.6f)).reduced(0, 0)); // reducedは不要になるかもだが一応0
}

//==============================================================================
void LooperTrackUi::mouseDown(const juce::MouseEvent& e)
{
	// 上部のボタンエリアのみクリック反応
	if (e.y < getWidth()) // 幅＝高さの正方形エリア
	{
		if(listener != nullptr)
			listener->trackClicked(this);
	}
}

void LooperTrackUi::mouseEnter(const juce::MouseEvent&){
	isMouseOver = true;
	repaint();
}
void LooperTrackUi::mouseExit(const juce::MouseEvent&){
	isMouseOver = false;
	repaint();
}

void LooperTrackUi::setSelected(bool shouldBeSelected){
	isSelected = shouldBeSelected;
	repaint();
}
bool LooperTrackUi::getIsSelected() const {return isSelected;}

void LooperTrackUi::setListener(Listener* listener) { this->listener = listener;}

void LooperTrackUi::setState(TrackState newState)
{
	if (state != newState)
    {
        state = newState;
        if (state == TrackState::Recording || state == TrackState::Playing || state == TrackState::Standby)
        {
            if (!isTimerRunning()) {
                flashProgress = 0.0f;
                startTimerHz(60);
            }
        }
        else
        {
            stopTimer(); // アニメーション用タイマー停止
        }
        repaint();
    }
}

juce::String LooperTrackUi::getStateString()const {
	switch (state) {
		case TrackState::Recording: return "Recording";
		case TrackState::Playing: return "Playing";
		case TrackState::Stopped: return "Stopped";
		case TrackState::Idle: return "Idle";
		case TrackState::Standby: return "Standby";
	}
	return "Unknown";
}

void LooperTrackUi::startRecording(){
	setState(TrackState::Recording);
}
void LooperTrackUi::stopRecording(){
	setState(TrackState::Playing);
}

void LooperTrackUi::startFlash(){
	isFlashing = true;
	flashProgress = 0.0f;
}

void LooperTrackUi::timerCallback(){
	// アニメーション更新
	if(state == TrackState::Recording || state == TrackState::Playing || state == TrackState::Standby){
		flashProgress += 0.02f;
		if(flashProgress >= 1.0f){
			flashProgress = 0.0f;
		}
		repaint();
	}
}

void LooperTrackUi::setLevel(float rms)
{
	currentRmsLevel = rms;
    // メーター部分の再描画だけでも良いが、アニメーションと同期しているので一括repaintでOK
	// 負荷が気になる場合は repaint(メーター領域) に限定も可能
}

void LooperTrackUi::drawGlowingBorder(juce::Graphics& g, juce::Colour glowColour)
{
    // 旧メソッド（互換性のため残すか、削除して新しい方を使う）
    drawGlowingBorder(g, glowColour, getLocalBounds().toFloat()); 
}

