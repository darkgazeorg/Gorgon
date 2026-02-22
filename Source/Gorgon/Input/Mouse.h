/// @file

#pragma once

#include "../Types.h"
#include "../Enum.h"

namespace Gorgon :: Input {

	namespace Mouse {

		/// The type of a mouse event. Out/Up occurs if Over/Down is handled. 
		/// Click occurs only if Down is not handled. 
		ENUMCLASS EventType {
            HitCheck, ///< Checks if the coordinate hits the layer, always called first
			Over,
			Move,
			MovePressed, ///< Move event while a button is pressed
			Out,
			Down,
			DownPressed, ///< Down event while a button is already pressed
			Click,
			Up,
            Scroll_Vert,
            Scroll_Hor,
            Zoom,
            Rotate
		};

        /// Lists the mouse button constants
		ENUMCLASS Button {
            None   = 0 ,
			Left   = 0b1000000001 ,
			Right  = 0b1000000010 ,
			Middle = 0b1000000100 ,
			X1	   = 0b1000001000 ,
			X2	   = 0b1000010000 ,
			X3	   = 0b1000100000 ,
			X4	   = 0b1001000000 ,
			X5	   = 0b1010000000 ,
			X6	   = 0b1100000000 ,
			All    = 0b1111111111,
		};
        
        inline Button operator |(Button l, Button r) {
            return Button((int)l | (int)r);
        }
        
        inline Button operator &(Button l, Button r) {
            return Button((int)l & (int)r);
        }
        
        inline bool operator &&(Button l, Button r) {
            return (int)(l&r) > 0;
        }
        
        inline Button operator ~(Button l) {
            return Button(~(unsigned)l);
        }

		DefineEnumStrings(
			Button, 
			{Button::Left, "Left"},
			{Button::Right, "Right"},
			{Button::Middle, "Middle"},
			{Button::X1, "X1"},
			{Button::X2, "X2"},
			{Button::X3, "X3"},
			{Button::X4, "X4"},
			{Button::X5, "X5"},
			{Button::X6, "X6"},
		);

		ENUMCLASS ScrollType {
			Vertical,
			Horizontal,
			Zoom
		};

	}

}
