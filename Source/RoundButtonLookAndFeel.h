/*
  ==============================================================================

    RoundButtonLookAndFeel.h
    Created: 16 Dec 2025
    Author:  mt sh

  ==============================================================================
*/

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// 🍕 丸型ボタン専用LookAndFeel
class RoundButtonLookAndFeel : public juce::LookAndFeel_V4
{
public:
	RoundButtonLookAndFeel() = default;
	
	void drawButtonBackground(juce::Graphics& g,
							juce::Button& button,
							const juce::Colour& backgroundColour,
							bool isMouseOverButton,
							bool isButtonDown) override
	{
		auto bounds = button.getLocalBounds().toFloat();
		
		// 幅を基準に円のサイズを決定（縦長ボタン前提）
		float diameter = bounds.getWidth();
		
		// 円を上部に配置（少しマージンを空ける）
		float x = bounds.getCentreX() - diameter * 0.5f;
		float y = bounds.getY() + 2.0f;
		
		juce::Rectangle<float> circleBounds(x, y, diameter, diameter);
		
		// マウスオーバー・押下時の明度調整
		auto fillColour = backgroundColour;
		if (isButtonDown)
			fillColour = fillColour.darker(0.3f);
		else if (isMouseOverButton)
			fillColour = fillColour.brighter(0.1f);
		
		// 円を描画
		g.setColour(fillColour);
		g.fillEllipse(circleBounds.reduced(2.0f));  // 少し小さくして余白を作る
		
		// 縁取り
		g.setColour(juce::Colours::black.withAlpha(0.3f));
		g.drawEllipse(circleBounds.reduced(2.0f), 1.5f);
		
		// ボタンの下にラベルテキストを表示
		auto& textButton = dynamic_cast<juce::TextButton&>(button);
		auto buttonText = textButton.getButtonText();
		
		juce::String labelText = "";
		
		// Unicodeシンボルからラベルテキストを決定
		if (buttonText == juce::String::fromUTF8("\xE2\x8F\xBA"))  // ⏺
			labelText = "REC";
		else if (buttonText == juce::String::fromUTF8("\xE2\x96\xA0"))  // ■
			labelText = "STOP";
		else if (buttonText == juce::String::fromUTF8("\xE2\x96\xB6"))  // ▶
			labelText = "PLAY";
		else if (buttonText == juce::String::fromUTF8("\xE2\x86\xB6"))  // ↶
			labelText = "UNDO";
		else if (buttonText == juce::String::fromUTF8("\xE2\x8C\xAB"))  // ⌫
			labelText = "CLEAR";
		else if (buttonText == juce::String::fromUTF8("\xE2\x9A\x99"))  // ⚙
			labelText = "SETUP";
		
		if (labelText.isNotEmpty())
		{
			g.setColour(juce::Colour::fromRGB(80, 60, 45));  // DeepOvenBrown
			g.setFont(10.0f);
			
			// ボタンの下に配置（円の下）
			juce::Rectangle<float> labelBounds(
				bounds.getX(),
				circleBounds.getBottom() + 2.0f,
				bounds.getWidth(),
				12.0f
			);
			
			g.drawText(labelText, labelBounds, juce::Justification::centred, true);
		}
	}
	
	void drawButtonText(juce::Graphics& g,
					   juce::TextButton& button,
					   bool isMouseOverButton,
					   bool isButtonDown) override
	{
		auto bounds = button.getLocalBounds().toFloat();
		// 円のサイズと位置を再計算
		float diameter = bounds.getWidth();
		float circleY = bounds.getY() + 2.0f;
		
		auto iconSize = diameter * 0.6f;
		auto centerX = bounds.getCentreX();
		auto centerY = circleY + diameter * 0.5f; // 円の中心
		
		auto text = button.getButtonText();
		
		// すべてのアイコンをCreamDough色（ベージュ）に統一
		g.setColour(PizzaColours::CreamDough);
		
		// Undoボタンの場合は90度回転
		if (text == juce::String::fromUTF8("\xE2\x86\xB6"))  // ↶
		{
			// 回転の中心を円の中心に設定
			juce::AffineTransform rotation = juce::AffineTransform::rotation(
				juce::MathConstants<float>::halfPi,
				centerX, 
				centerY
			);
			
			g.addTransform(rotation);
			
			// 矢印を描画（パスで描画）
			juce::Path arrow;
			// サイズを調整
			float arrowSize = iconSize * 0.85f;
			
			// 円弧を描画（3/4周）
			arrow.addArc(centerX - arrowSize/2, centerY - arrowSize/2, 
						arrowSize, arrowSize, 
						juce::MathConstants<float>::pi * 0.6f,  // 開始角度を調整
						juce::MathConstants<float>::pi * 2.1f,  // 終了角度を調整
						true);
						
			// ストロークを太く（0.2f -> 0.28f）
			g.strokePath(arrow, juce::PathStrokeType(arrowSize * 0.28f));
			
			// 矢印の先端（より大きく、はっきりと）
			juce::Path arrowHead;
			float headSize = arrowSize * 0.5f;  // 0.4f -> 0.5f に拡大
			float headWidth = arrowSize * 0.35f; // 幅も調整
			
			// 矢印ヘッドの位置を調整（円弧の終点）
			float headCenterX = centerX - arrowSize/2;
			float headCenterY = centerY;
			
			arrowHead.addTriangle(
				headCenterX, headCenterY,
				headCenterX + headSize, headCenterY - headWidth/2,
				headCenterX + headSize, headCenterY + headWidth/2
			);
			g.fillPath(arrowHead);
		}
		else if (text == juce::String::fromUTF8("\xE2\x8F\xBA"))  // ⏺ RECボタン
		{
			// 小さな塗りつぶした円を描画
			float circleRadius = iconSize / 3;
			g.fillEllipse(centerX - circleRadius, centerY - circleRadius, 
						  circleRadius * 2, circleRadius * 2);
		}
		else if (text == juce::String::fromUTF8("\xE2\x96\xA0"))  // ■ STOPボタン
		{
			// 角丸四角を描画
			float squareSize = iconSize * 0.8f;
			juce::Rectangle<float> squareArea(
				centerX - squareSize/2,
				centerY - squareSize/2,
				squareSize,
				squareSize
			);
			g.fillRoundedRectangle(squareArea, squareSize * 0.15f); // 角を少し丸く
		}
		else
		{
			// その他のテキスト描画（三角形、X、歯車など）
			juce::Rectangle<float> iconArea(
				centerX - iconSize/2,
				centerY - iconSize/2,
				iconSize,
				iconSize
			);
			
			g.setFont(iconSize); 
			g.drawText(text, iconArea, juce::Justification::centred, false);
		}
	}
};
