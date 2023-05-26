module wobbl_e_character_render(
	input [7:0] character,
	input [3:0] row,
	input [3:0] col,
	output pixel);
	
	wire [25:0] lut_output;
	
  assign pixel = (lut_output >> ((4-row)*5 + (4-col)));
  
  	/*initial begin
      $monitor("lut value: %b",  lut_output);
      $monitor("shifted lut value: %b",  (lut_output << (4-row)*5 + (4-col)));
      $monitor("masked lut value: %b",  1 & (lut_output << (4-row)*5 + (4-col)));
    end*/
	
  	always @(character)
      begin
        
      case (character)
		// space
			8'd32: lut_output = {
				5'b00000,
				5'b00000,
				5'b00000,
				5'b00000,
				5'b00000};
		// colon
			8'd58: lut_output = {
				5'b00000,
				5'b01000,
				5'b00000,
				5'b01000,
				5'b00000};
		// A-Z
			//a
			8'd65: lut_output = {
				5'b01110,
				5'b10001,
				5'b11111,
				5'b10001,
				5'b10001};
			8'd66: lut_output = {
				5'b11110,
				5'b10001,
				5'b11110,
				5'b10001,
				5'b11110};
			8'd67: lut_output = {
				5'b01111,
				5'b10000,
				5'b10000,
				5'b10000,
				5'b01111};
			8'd68: lut_output = {
				5'b11110,
				5'b10001,
				5'b10001,
				5'b10001,
				5'b11110};
			8'd69: lut_output = {
				5'b11111,
				5'b10000,
				5'b11111,
				5'b10000,
				5'b11111};
			8'd70: lut_output = {
				5'b11111,
				5'b10000,
				5'b11111,
				5'b10000,
				5'b10000};
			// g
			8'd71: lut_output = {
				5'b01111,
				5'b10000,
				5'b10011,
				5'b10001,
				5'b01110};
			8'd72: lut_output = {
				5'b10001,
				5'b10001,
				5'b11111,
				5'b10001,
				5'b10001};
			8'd73: lut_output = {
				5'b11111,
				5'b00100,
				5'b00100,
				5'b00100,
				5'b11111};
			8'd74: lut_output = {
				5'b11111,
				5'b00001,
				5'b00001,
				5'b10001,
				5'b01110};
			8'd75: lut_output = {
				5'b10001,
				5'b10010,
				5'b11100,
				5'b10010,
				5'b10001};
			8'd76: lut_output = {
				5'b10000,
				5'b10000,
				5'b10000,
				5'b10000,
				5'b11111};
			// m
			8'd77: lut_output = {
				5'b10001,
				5'b11011,
				5'b10101,
				5'b10001,
				5'b10001};
			8'd78: lut_output = {
				5'b11001,
				5'b11011,
				5'b10101,
				5'b10011,
				5'b10011};
			8'd79: lut_output = {
				5'b01110,
				5'b10001,
				5'b10001,
				5'b10001,
				5'b01110};
			8'd80: lut_output = {
				5'b11111,
				5'b10001,
				5'b11111,
				5'b10000,
				5'b10000};
			8'd81: lut_output = {
				5'b01110,
				5'b10001,
				5'b10001,
				5'b10010,
				5'b01101};
			8'd82: lut_output = {
				5'b11110,
				5'b10001,
				5'b11110,
				5'b10001,
				5'b10001};
			// s
			8'd83: lut_output = {
				5'b01111,
				5'b10000,
				5'b01110,
				5'b00001,
				5'b11110};
			8'd84: lut_output = {
				5'b11111,
				5'b00100,
				5'b00100,
				5'b00100,
				5'b00100};
			8'd85: lut_output = {
				5'b10001,
				5'b10001,
				5'b10001,
				5'b10001,
				5'b01110};
			8'd86: lut_output = {
				5'b10001,
				5'b10001,
				5'b01010,
				5'b01010,
				5'b00100};
			8'd87: lut_output = {
				5'b10001,
				5'b10001,
				5'b10101,
				5'b11011,
				5'b10001};
			8'd88: lut_output = {
				5'b10001,
				5'b01010,
				5'b00100,
				5'b01010,
				5'b10001};
			// y
			8'd89: lut_output = {
				5'b10001,
				5'b10001,
				5'b01010,
				5'b00100,
				5'b00100};
			8'd90: lut_output = {
				5'b11111,
				5'b00011,
				5'b00100,
				5'b11000,
				5'b11111};
		// 0-9
			8'd48: lut_output = {
				  5'b11111,
				  5'b10001,
				  5'b10001,
				  5'b10001,
				  5'b11111};
			 8'd49: lut_output = {
				  5'b00100,
				  5'b01100,
				  5'b00100,
				  5'b00100,
				  5'b11111};
			 8'd50: lut_output = {
				  5'b11111,
				  5'b00001,
				  5'b11111,
				  5'b10000,
				  5'b11111};
			 8'd51: lut_output = {
				  5'b11111,
				  5'b00001,
				  5'b11111,
				  5'b00001,
				  5'b11111};
			 8'd52: lut_output = {
				  5'b10000,
				  5'b10000,
				  5'b10100,
				  5'b11111,
				  5'b00100};
			 8'd53: lut_output = {
				  5'b11111,
				  5'b10000,
				  5'b11110,
				  5'b00001,
				  5'b11110};
			 8'd54: lut_output = {
				  5'b01110,
				  5'b10000,
				  5'b11111,
				  5'b10001,
				  5'b11111};
			 8'd55: lut_output = {
				  5'b11111,
				  5'b00001,
				  5'b00010,
				  5'b00100,
				  5'b01000};
			 8'd56: lut_output = {
				  5'b01110,
				  5'b10001,
				  5'b01110,
				  5'b10001,
				  5'b01110};
			 8'd57: lut_output = {
				  5'b11111,
				  5'b10001,
				  5'b11111,
				  5'b00001,
				  5'b00001};
			 default: lut_output = {
				  5'b01010,
				  5'b01010,
				  5'b00000,
				  5'b10001,
				  5'b01110};
			endcase;
      end
	
endmodule
