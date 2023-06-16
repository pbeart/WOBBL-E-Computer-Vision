module EEE_IMGPROC(
	// global clock & reset
	clk,
	reset_n,
	
	// mm slave
	s_chipselect,
	s_read,
	s_write,
	s_readdata,
	s_writedata,
	s_address,

	// stream sink
	sink_data,
	sink_valid,
	sink_ready,
	sink_sop,
	sink_eop,
	
	// streaming source
	source_data,
	source_valid,
	source_ready,
	source_sop,
	source_eop,
	
	// conduit
	mode
	
);


// global clock & reset
input	clk;
input	reset_n;

// mm slave
input							s_chipselect;
input							s_read;
input							s_write;
output	reg	[31:0]	s_readdata;
input	[31:0]				s_writedata;
input	[2:0]					s_address;


// streaming sink
input	[23:0]            	sink_data;
input								sink_valid;
output							sink_ready;
input								sink_sop;
input								sink_eop;

// streaming source
output	[23:0]			  	   source_data;
output								source_valid;
input									source_ready;
output								source_sop;
output								source_eop;

// conduit export
input                         mode;

////////////////////////////////////////////////////////////////////////
//
parameter IMAGE_W = 11'd640;
parameter IMAGE_H = 11'd480;
parameter MESSAGE_BUF_MAX = 256;
parameter MSG_INTERVAL = 6;
parameter BB_COL_DEFAULT = 24'h00ff00;
parameter THRESH_DEFAULT = 16'h10;
parameter PIXTHRESH_DEFAULT = 16'h7f;

wire unsigned [7:0]   red, green, blue, grey;
wire unsigned [7:0]   red_out, green_out, blue_out;

wire         sop, eop, in_valid, out_ready;
////////////////////////////////////////////////////////////////////////

// Detect red areas
wire match_detect;

// Original red-detection
//assign red_detect = red[7] & ~green[7] & ~blue[7];

wire [7:0] target_red, target_green, target_blue;
//assign target_red = 8'hff;
//assign target_green = 8'h00;
//assign target_blue = 8'hff;

assign target_blue = bb_col[7:0];
assign target_green = bb_col[15:8];
assign target_red = bb_col[23:16];

wire signed [8:0] signed_red, signed_green,signed_blue;
wire signed [8:0] signed_target_red, signed_target_green,signed_target_blue;

assign signed_red = red;
assign signed_green = green;
assign signed_blue = blue;

assign signed_target_red = target_red;
assign signed_target_green = target_green;
assign signed_target_blue = target_blue;


wire signed [10:0] red_subtracted;
assign red_subtracted = (signed_target_red - signed_red);

wire signed [10:0] green_subtracted;
assign green_subtracted = (signed_target_green  - signed_green);

wire signed [10:0] blue_subtracted;
assign blue_subtracted = (signed_target_blue - signed_blue);

// max result of these is 3 * (8bit * 8bit), which requires 18 bits

wire signed [23:0] distance_squared;
wire signed [23:0] red_squared, green_squared, blue_squared;

assign red_squared = red_subtracted*red_subtracted;//red_subtracted*red_subtracted;
assign green_squared = green_subtracted*green_subtracted;//green_subtracted*green_subtracted;
assign blue_squared = blue_subtracted*blue_subtracted;//blue_subtracted*blue_subtracted;

assign distance_squared = ((red_squared<0 ? -red_squared : red_squared) + (green_squared<0 ? -green_squared : green_squared) + (blue_squared<0 ? -blue_squared: blue_squared));


assign match_detect = (distance_squared < thresh) && within_border;

wire[10:0] border_encroaches;

assign border_encroaches = 10'd20;


// the pixel with the lowest distance_squared
reg[10:0] matchiest_pixel_x, matchiest_pixel_y, crosshair_x, crosshair_y;

reg[21:0] match_count;
reg[10:0] last_average_match_x, last_average_match_y;
reg[31:0] total_match_x, total_match_y;

// the distance of this pixel
reg[18:0] matchiest_pixel;

// Find boundary of cursor box

// Highlight detected areas
wire [23:0] red_high, with_target_explainer, with_target_crosshair, with_avg_crosshair, with_text, with_uart_activity, with_border;
assign grey = green[7:1] + red[7:2] + blue[7:2]; //Grey = green/2 + red/4 + blue/4


assign red_high  =  match_detect ? {8'hff, 8'h0, 8'hff} : {red, green, blue};

wire show_avg_crosshair;

assign show_avg_crosshair = (((x + 1 > filtered_blob_average_x) && (x < filtered_blob_average_x + 1) && (y < filtered_blob_average_y + 80) && (y + 80 > filtered_blob_average_y)) ||
((y +1 > filtered_blob_average_y) && (y < filtered_blob_average_y + 1) && (x < filtered_blob_average_x + 80) && (x + 80 > filtered_blob_average_x)));

assign with_avg_crosshair = show_avg_crosshair ? 24'h7fff7f : red_high;

wire within_border;

assign within_border = (x > border_encroaches && x < (IMAGE_W - border_encroaches) && y > border_encroaches && y<(IMAGE_H - border_encroaches));

assign with_border = (x == border_encroaches || x == (IMAGE_W - border_encroaches) || y == border_encroaches || y == (IMAGE_H - border_encroaches)) ? (24'h0000FF) : with_avg_crosshair;


wire show_crosshair;

assign show_crosshair = (((x == last_average_match_x) && (y < last_average_match_y + 20) && (y + 20 > last_average_match_y)) ||
((y == last_average_match_y) && (x < last_average_match_x + 20) && (x + 20 > last_average_match_x)));

wire show_sampler;

assign show_sampler = (x == sampler_x && y == sampler_y);



assign with_target_crosshair = show_sampler ? 24'hFFFFFF : (show_crosshair ? 24'hff0000 : with_border);



wire[3:0] fps_high;
wire[3:0] fps_low;

wire bad_vs_analogue;

assign bad_vs_analogue = y==0;

// instantiate an FPS Counter
FpsCounter fps_counter(
	.clk50(clk),
	.vs(bad_vs_analogue),
	
	.fps_h(fps_high),
	.fps_l(fps_low)
);

wire [7:0] displaystring [19:0];
assign displaystring[0] = "C";
assign displaystring[1] = "O";
assign displaystring[2] = "L";
assign displaystring[3] = ":";
assign displaystring[4] = " ";
assign displaystring[5] = " ";
assign displaystring[6] = " ";
assign displaystring[7] = "F";
assign displaystring[8] = "P";
assign displaystring[9] = "S";
assign displaystring[10] = ":";
assign displaystring[11] = fps_high+8'd48;
assign displaystring[12] = fps_low+8'd48;
assign displaystring[13] = " ";
assign displaystring[14] = "R";
assign displaystring[15] = "C";
assign displaystring[16] = "V";
assign displaystring[17] = ":"; //(uart_active_count)+8'd48;//":";
assign displaystring[18] = " ";
assign displaystring[19] = " ";


wire [7:0] current_string_index = x / (char_spacing_x + 5*char_size);
wire [7:0] current_char;

assign current_char = displaystring[current_string_index];

localparam [4:0] char_size=3; 
localparam [4:0] char_spacing_x = 1;
localparam [4:0] char_spacing_y = 5;

wire [4:0] char_col;
assign char_col = (x % (char_spacing_x + 5*char_size))/char_size;

wire [4:0] char_row;
assign char_row = (y % (char_spacing_y + 5*char_size))/char_size;

wire char_pixel;

assign with_text = (y<=480 && y>=460 && x <  20  *  (char_spacing_x + 5*char_size)) ? (char_pixel && (char_col <= 4 && char_row <= 4) ? 24'hFFFFFF : 24'h000000) : with_target_crosshair;

assign with_target_explainer = ((x<96) && (x>64) && (y<=480) && (y>=460)) ? {target_red, target_green, target_blue } : with_text;

wire uart_was_active;

reg [31:0] uart_active_count;

assign uart_was_active = uart_active_count > 4'd0;

assign with_uart_activity = ((x<320) && (x>288) && (y<=480) && (y>=460)) ? (uart_was_active ? 24'hFFFFFF : 24'h000000) : with_target_explainer;


wobbl_e_character_render text_renderer(
    .character(current_char),
    .row(char_row),
    .col(char_col),
    .pixel(char_pixel));


// Show bounding box
wire [23:0] new_image;
wire bb_active;
assign bb_active = (x == left) | (x == right) | (y == top) | (y == bottom);
assign new_image = with_uart_activity; //bb_active ? bb_col : with_target_explainer;

// Switch output pixels depending on mode switch
// Don't modify the start-of-packet word - it's a packet discriptor
// Don't modify data in non-video packets
assign {red_out, green_out, blue_out} = (mode & ~sop & packet_video) ? new_image : {red,green,blue};



//Count valid pixels to tget the image coordinates. Reset and detect packet type on Start of Packet.
reg [10:0] x, y;
reg packet_video;
always@(posedge clk) begin
	if (sop) begin
		x <= 11'h0;
		y <= 11'h0;
		packet_video <= (blue[3:0] == 3'h0);
	end
	else if (in_valid) begin
		if (x == sampler_x && y == sampler_y) begin
			sampler_red <= red;
			sampler_green <= green;
			sampler_blue <= blue;
		end
		if (x == IMAGE_W-1) begin
			x <= 11'h0;
			y <= y + 11'h1;
		end
		else begin
			x <= x + 11'h1;
		end
	end
end

reg [7:0] cursor_red;
reg [7:0] cursor_green;
reg [7:0] cursor_blue;


wire [10:0] sampler_x, sampler_y;

assign sampler_x = 11'd320;
assign sampler_y = 11'd240;

reg [7:0] sampler_red, sampler_green, sampler_blue;

reg [10:0] filtered_blob_average_x, filtered_blob_average_y;


//Find first and last red pixels
reg [10:0] x_min, y_min, x_max, y_max;
reg[23:0] weight_pix_thresh;
always@(posedge clk) begin
	if (match_detect & in_valid) begin	//Update bounds when the pixel is red
		if (x < x_min) x_min <= x;
		if (x > x_max) x_max <= x;
		if (y < y_min) y_min <= y;
		
		y_max <= y;
		total_match_x<=total_match_x + x;
		total_match_y<=total_match_y + y;
		if (!sop) begin
			match_count<=match_count + 1;
		end
	end
	if (distance_squared <= matchiest_pixel && (x>20) && (y>30) && (x< IMAGE_W-30) && (y<IMAGE_H-30)) begin
		matchiest_pixel_x<=x;
		matchiest_pixel_y<=y;
		matchiest_pixel <= distance_squared;
		cursor_red<=red;
		cursor_green<=green;
		cursor_blue<=blue;
	end
	if (sop & in_valid) begin	//Reset bounds on start of packet
		x_min <= IMAGE_W-11'h1;
		x_max <= 0;
		y_min <= IMAGE_H-11'h1;
		y_max <= 0;
		match_count<=0;
		
		// to avoid multiple immediately repeated 'start of packets' causing us to lose the crosshair position! (really should debounce)
		if (matchiest_pixel_y != 0) begin
			//crosshair_x<=matchiest_pixel_x;
			//crosshair_y<=matchiest_pixel_y;
			
			last_average_match_x<=(total_match_x / match_count);
			last_average_match_y<=(total_match_y / match_count);
			if (match_count>weight_pix_thresh) begin
				filtered_blob_average_x<=4*filtered_blob_average_x/5 + last_average_match_x/5;
				filtered_blob_average_y<=4*filtered_blob_average_y/5 + last_average_match_y/5;
			end
		end
		
		matchiest_pixel_x <= 0;
		matchiest_pixel_y <= 0;
		total_match_x<=0;
		total_match_y<=0;
		matchiest_pixel <= {19{1'b1}};
	end
end

//Process bounding box at the end of the frame.
reg [1:0] msg_state;
reg [10:0] left, right, top, bottom;
reg [7:0] frame_count;
always@(posedge clk) begin
	if (eop & in_valid & packet_video) begin  //Ignore non-video packets
		
		//Latch edges for display overlay on next frame
		left <= x_min;
		right <= x_max;
		top <= y_min;
		bottom <= y_max;
		
		
		//Start message writer FSM once every MSG_INTERVAL frames, if there is room in the FIFO
		frame_count <= frame_count - 1;
		
		if (frame_count == 0 && msg_buf_size < MESSAGE_BUF_MAX - 3) begin
			msg_state <= 2'b01;
			frame_count <= MSG_INTERVAL-1;
		end
	end
	
	//Cycle through message writer states once started
	if (msg_state != 2'b00) msg_state <= msg_state + 2'b01;

end
	
//Generate output messages for CPU
reg [31:0] msg_buf_in; 
wire [31:0] msg_buf_out;
reg msg_buf_wr;
wire msg_buf_rd, msg_buf_flush;
wire [7:0] msg_buf_size;
wire msg_buf_empty;

`define RED_BOX_MSG_ID "RBB"

always@(*) begin	//Write words to FIFO as state machine advances
	case(msg_state)
		2'b00: begin
			msg_buf_in = 32'b0;
			msg_buf_wr = 1'b0;
		end
		2'b01: begin
			msg_buf_in = `RED_BOX_MSG_ID;	//Message ID
			msg_buf_wr = 1'b1;
		end
		2'b10: begin
			//msg_buf_in = {5'b0, x_min, 5'b0, y_min};	//Top left coordinate
			// crosshair x and y are 11 bit each
			msg_buf_in = {5'b0, filtered_blob_average_x, 5'b0, filtered_blob_average_y};
			msg_buf_wr = 1'b1;
		end
		2'b11: begin
			//msg_buf_in = {5'b0, x_max, 5'b0, y_max}; //Bottom right coordinate
			
			//msg_buf_in = {13'b0, matchiest_pixel};
			//msg_buf_in = {8'b0, cursor_red, cursor_green, cursor_blue};
			//msg_buf_in = {8'b0, target_red, target_green, target_blue};
			//msg_buf_in = uart_active_count;
			//msg_buf_in = 31'hDEADBEEF;
			msg_buf_in = {10'b0, match_count};
			msg_buf_wr = 1'b1;
		end
	endcase
end


//Output message FIFO
MSG_FIFO	MSG_FIFO_inst (
	.clock (clk),
	.data (msg_buf_in),
	.rdreq (msg_buf_rd),
	.sclr (~reset_n | msg_buf_flush),
	.wrreq (msg_buf_wr),
	.q (msg_buf_out),
	.usedw (msg_buf_size),
	.empty (msg_buf_empty)
	);


//Streaming registers to buffer video signal
STREAM_REG #(.DATA_WIDTH(26)) in_reg (
	.clk(clk),
	.rst_n(reset_n),
	.ready_out(sink_ready),
	.valid_out(in_valid),
	.data_out({red,green,blue,sop,eop}),
	.ready_in(out_ready),
	.valid_in(sink_valid),
	.data_in({sink_data,sink_sop,sink_eop})
);

STREAM_REG #(.DATA_WIDTH(26)) out_reg (
	.clk(clk),
	.rst_n(reset_n),
	.ready_out(out_ready),
	.valid_out(source_valid),
	.data_out({source_data,source_sop,source_eop}),
	.ready_in(source_ready),
	.valid_in(in_valid),
	.data_in({red_out, green_out, blue_out, sop, eop})
);


/////////////////////////////////
/// Memory-mapped port		 /////
/////////////////////////////////

// Addresses
`define REG_STATUS    			0
`define READ_MSG    				1
`define READ_ID    				2
`define REG_BBCOL					3
`define REG_THRESH				4
`define REG_PIXTHRESH			5

//Status register bits
// 31:16 - unimplemented
// 15:8 - number of words in message buffer (read only)
// 7:5 - unused
// 4 - flush message buffer (write only - read as 0)
// 3:0 - unused


// Process write

reg  [7:0]   reg_status;
reg	[23:0]	bb_col;
reg [31:0] thresh;

always @ (posedge clk)
begin
	if (~reset_n)
	begin
		reg_status <= 8'b0;
		bb_col <= BB_COL_DEFAULT;
		thresh <= THRESH_DEFAULT;
		weight_pix_thresh <= PIXTHRESH_DEFAULT;
		uart_active_count<=0;
	end
	else begin
		if(s_chipselect & s_write) begin
			uart_active_count<=32'h00FFFFFF;
		   if      (s_address == `REG_STATUS)	reg_status <= s_writedata[7:0];
		   if      (s_address == `REG_BBCOL)	bb_col <= s_writedata[23:0];
			if   		(s_address == `REG_THRESH) thresh <= s_writedata[31:0];
			if   		(s_address == `REG_PIXTHRESH) weight_pix_thresh <= s_writedata[31:0];
		end
		else begin
			if (uart_active_count>4'h0) begin 
				uart_active_count<=uart_active_count-1;
			end
		end
	end
end


//Flush the message buffer if 1 is written to status register bit 4
assign msg_buf_flush = (s_chipselect & s_write & (s_address == `REG_STATUS) & s_writedata[4]);


// Process reads
reg read_d; //Store the read signal for correct updating of the message buffer

// Copy the requested word to the output port when there is a read.
always @ (posedge clk)
begin
   if (~reset_n) begin
	   s_readdata <= {32'b0};
		read_d <= 1'b0;
	end
	
	else if (s_chipselect & s_read) begin
		if   (s_address == `REG_STATUS) s_readdata <= {16'b0,msg_buf_size,reg_status};
		if   (s_address == `READ_MSG) s_readdata <= {msg_buf_out};
		if   (s_address == `READ_ID) s_readdata <= 32'h1234EEE2;
		if   (s_address == `REG_BBCOL) s_readdata <= {8'h0, bb_col};
		if   (s_address == `REG_THRESH) s_readdata <= {thresh};
		if   (s_address == `REG_PIXTHRESH) s_readdata <= {weight_pix_thresh};
	end
	
	read_d <= s_read;
end

//Fetch next word from message buffer after read from READ_MSG
assign msg_buf_rd = s_chipselect & s_read & ~read_d & ~msg_buf_empty & (s_address == `READ_MSG);
						


endmodule

