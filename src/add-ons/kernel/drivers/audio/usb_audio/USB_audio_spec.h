/*
 *	USB audio spec stuctures
 *
 *  Based on the USB Device Class Definition for Audio Devices Release 1.0
 *  (March 18, 1998)
 *
 *  And the USB Device Class Definition for Audio Formats Release 1.0
 *  (March 18, 1998)
 */
#ifndef __USB_AUDIO_SPEC_H__
#define __USB_AUDIO_SPEC_H__

#include <SupportDefs.h>

#ifdef __cplusplus
extern "C" {
#endif

// Audio Control descriptors
// header descriptor
/*typedef struct {
	uint8	length;
	uint8	descriptor_type;		// CS_INTERFACE descriptor type (0x24)
	uint8	descriptor_subtype; 	// HEADER
	uint16	bcd_release_no;			// Audio Device Class Specification relno
	uint16	total_length;			//
	uint8	in_collection;			// # of audiostreaming units
	uint8	interface_numbers[1];	// or more
} _PACKED usb_audiocontrol_header_descriptor_r1;*/

typedef struct {
	uint8	length;
	uint8	descriptor_type;		// CS_INTERFACE descriptor type (0x24)
	uint8	descriptor_subtype; 	// HEADER
	uint16	bcd_release_no;			// Audio Device Class Specification relno
	union {
		struct {
			uint16	total_length;			//
			uint8	in_collection;			// # of audiostreaming units
			uint8	interface_numbers[1];	// or more
		} _PACKED r1;

		struct {
			uint8	function_category;		// this Audio function Category
			uint16	total_length;			//
			uint8	bm_controls;			// bitmap of controls
		} _PACKED r2;

	};

} _PACKED usb_audiocontrol_header_descriptor;

// input terminal descriptor
// Table 4-3, page 39
/*typedef struct {
	uint8	length;
	uint8	descriptor_type;		// CS_INTERFACE descriptor type (0x24)
	uint8	descriptor_subtype; 	// INPUT_TERMINAL
	uint8	terminal_id;			//
	uint16	terminal_type;			// 0x0101 ?
	uint8	assoc_terminal;			// OT terminal ID of corresponding outp
	uint8	num_channels;			// stereo = 2
	uint8	channel_config;			// spatial location of two channels
									//		(bitmap 0x03 is plain stereo)
	uint8	channel_names;			// index of string descr,
									//		name of first logical channel
	uint8	terminal;				// index of string descr,
									//		name of Input Terminal
} _PACKED usb_input_terminal_descriptor_r1; */

// Table 4-9, page 53
typedef struct {
	uint8	length;
	uint8	descriptor_type;		// CS_INTERFACE descriptor type (0x24)
	uint8	descriptor_subtype; 	// INPUT_TERMINAL
	uint8	terminal_id;			//
	uint16	terminal_type;			// 0x0101 ?
	uint8	assoc_terminal;			// OT terminal ID of corresponding outp

	union {
		struct {
			uint8	num_channels;			// stereo = 2
			uint8	channel_config;			// spatial location of two channels
											//		(bitmap 0x03 is plain stereo)
			uint8	channel_names;			// index of string descr,
											//		name of first logical channel
			uint8	terminal;				// index of string descr,
											//		name of Input Terminal
		} _PACKED r1;

		struct {
			uint8	clock_source_id;		//
			uint8	num_channels;			// stereo = 2
			uint32	channel_config;			// spatial location of two channels
											//		(bitmap 0x03 is plain stereo)
			uint8	channel_names;			// index of string descr,
											//		name of first logical channel
			uint16	bm_controls;			//
			uint8	terminal;				// index of string descr,
											//		name of Input Terminal
		} _PACKED r2;

	};
} _PACKED usb_input_terminal_descriptor;

// output terminal descriptor
// Table 4-4, page 40
/*typedef struct {
	uint8	length;
	uint8	descriptor_type;		// CS_INTERFACE descriptor type (0x24)
	uint8	descriptor_subtype; 	// OUTPUT_TERMINAL
	uint8	terminal_id;			//
	uint16	terminal_type;			// 0x0101 ?
	uint8	assoc_terminal;			// OT terminal ID of corresponding outp
	uint8	source_id;				// ID of the unit or terminal to which
									//		this terminal is connected
	uint8	terminal;		// index of string descr,
									//		name of Input Terminal
} _PACKED usb_output_terminal_descriptor_r1; */

typedef struct {
	uint8	length;
	uint8	descriptor_type;		// CS_INTERFACE descriptor type (0x24)
	uint8	descriptor_subtype; 	// OUTPUT_TERMINAL
	uint8	terminal_id;			//
	uint16	terminal_type;			// 0x0101 ?
	uint8	assoc_terminal;			// OT terminal ID of corresponding outp
	uint8	source_id;				// ID of the unit or terminal to which
									//		this terminal is connected
	union {
		struct {
			uint8	terminal;		// index of string descr,
									//		name of Input Terminal
		} _PACKED r1;

		struct {
			uint8	clock_source_id;//
			uint16	bm_controls;	//
			uint8	terminal;		// index of string descr,
									//		name of Input Terminal
		} _PACKED r2;

	};
} _PACKED usb_output_terminal_descriptor;


// mixer unit descriptor
// Table 4-5, page 41
typedef struct {
	uint8	length;
	uint8	descriptor_type;		// CS_INTERFACE descriptor type (0x24)
	uint8	descriptor_subtype; 	// MIXER_UNIT
	uint8	unit_id;				// unique within audio function
	uint8	num_input_pins;			//
	uint8	input_pins[1];			// array of source ids for the mixer
									// use usb_output_channels_descriptor
									// to parse the rest
} _PACKED usb_mixer_unit_descriptor;

// pseudo-descriptor for a section corresponding to logical output channels
// used in mixer, processing and extension descriptions.
typedef struct {
	uint8	num_output_pins; 		// number of mixer output pins
	uint16	channel_config;		 	// location of logical channels
	uint8	channel_names;		 	// id of name string of first logical channel
} _PACKED usb_output_channels_descriptor_r1;

typedef struct {
	uint8	num_output_pins; 		// number of mixer output pins
	uint32	channel_config;		 	// location of logical channels
	uint8	channel_names;		 	// id of name string of first logical channel
} _PACKED usb_output_channels_descriptor;

// selector unit descriptor
// Table 4-6, page 43
typedef struct {
	uint8	length;
	uint8	descriptor_type;		// CS_INTERFACE descriptor type (0x24)
	uint8	descriptor_subtype; 	// SELECTOR_UNIT
	uint8	unit_id;				// unique within audio function
	uint8	num_input_pins;			//
	uint8	input_pins[1];			// id of the unit or terminal
									//		this pin is connected to
/*	uint8	selector_string;	*/	// be afraid of the variable
									//		size of input_pins array!
} _PACKED usb_selector_unit_descriptor;

// feature unit descriptor
// Table 4-7, page 43
/*typedef struct {
	uint8	length;
	uint8	descriptor_type;		// CS_INTERFACE descriptor type (0x24)
	uint8	descriptor_subtype; 	// FEATURE_UNIT
	uint8	unit_id;				// unique within audio function
	uint8	source_id;				// id of the unit or terminal to
									//		which this unit is connected
	uint8	control_size;			// size of element in bma_controls array
	uint8	bma_controls[1];		// the size of element must be equal
									//		to control_size!!
									// 	the channel 0 is master one!
/ *	uint8	feature_string;		* /  // be afraid of the variable size
									//		of bma_controls array!
} _PACKED usb_feature_unit_descriptor_r1; */

typedef struct {
	uint8	length;
	uint8	descriptor_type;		// CS_INTERFACE descriptor type (0x24)
	uint8	descriptor_subtype; 	// FEATURE_UNIT
	uint8	unit_id;				// unique within audio function
	uint8	source_id;				// id of the unit or terminal to
									//		which this unit is connected
	union {
		struct {
			uint8	control_size;			// size of element in bma_controls array
			uint8	bma_controls[1];		// the size of element must be equal
											//		to control_size!!
											// 	the channel 0 is master one!
		/*	uint8	feature_string;		*/  // be afraid of the variable size
											//		of bma_controls array!
		} _PACKED r1;

		struct {
			uint32	bma_controls[1];		// the channel 0 is master one!
		/*	uint8	feature_string;		*/  // be afraid of the variable size of
											//		bma_controls array!
		} _PACKED r2;
		
	};
} _PACKED usb_feature_unit_descriptor;

// processing unit descriptor
// Table 4-8, page 45
typedef struct {
	uint8	length;
	uint8	descriptor_type;		// CS_INTERFACE descriptor type (0x24)
	uint8	descriptor_subtype; 	// PROCESSING_UNIT
	uint8	unit_id;				// unique within audio function
	uint16	process_type;			// type of processing this unit is performing
	uint8	num_input_pins;			// number of input pins of this unit
	uint8	input_pins[1];			// array of source ids for the processing unit
		// use usb_output_channels_descriptor
		// to parse the rest
// TODO - the bmControl!!!!
} _PACKED usb_processing_unit_descriptor;

// extension unit descriptor
// Table 4-15, page 56
typedef struct {
	uint8	length;
	uint8	descriptor_type;		// CS_INTERFACE descriptor type (0x24)
	uint8	descriptor_subtype; 	// EXTENSION_UNIT
	uint8	unit_id;				// unique within audio function
	uint16	extension_code;			// vendor-specific code identifying this unit
	uint8	num_input_pins;			// number of input pins
	uint8	input_pins[1];			// array of source ids for the processing unit
		// use usb_output_channels_descriptor
		// to parse the rest
} _PACKED usb_extension_unit_descriptor;


// Audio Streaming (as) descriptors


// Class-specific AS interface descriptor
// Table 4-19, page 60
/*typedef struct {
	uint8	length;					// 7
	uint8	descriptor_type;		// CS_INTERFACE descriptor type (0x24)
	uint8	descriptor_subtype; 	// UAS_AS_GENERAL
	uint8	terminal_link;			// terminal ID to which this endp is connected
	uint8	delay;					// delay in # of frames
	uint16	format_tag;				// wFormatTag, 0x0001 = PCM
} _PACKED usb_as_interface_descriptor_r1;*/

typedef struct {
	uint8	length;					// 7
	uint8	descriptor_type;		// CS_INTERFACE descriptor type (0x24)
	uint8	descriptor_subtype; 	// UAS_AS_GENERAL
	uint8	terminal_link;			// terminal ID to which this endp is connected
	union {
		struct {
			uint8	delay;					// delay in # of frames
			uint16	format_tag;				// wFormatTag, 0x0001 = PCM
		} _PACKED r1;

		struct {
			uint8	bm_controls;			// controls bitmap
			uint8	format_type;			// type of audio streaming use
			uint32	bm_formats;				// audio data formats to be used with
											//		this interface
			uint8	num_output_pins; 		// number of physical channels in the claster
			uint32	channel_config;		 	// spatial location of channels
			uint8	channel_names;		 	// id of name string of first physical channel
		} _PACKED r2;

	};
} _PACKED usb_as_interface_descriptor;

// Class-specific As Isochronous Audio Data Endpoint descriptor
// Table 4-21, page 62
typedef struct {
	uint8	length;					// 7
	uint8	descriptor_type;		// UAS_CS_ENDPOINT descriptor type (0x25)
	uint8	descriptor_subtype; 	// UAS_EP_GENERAL
	uint8	attributes;				// d0 = samfq d1 = pitch d7 = maxpacketsonly
	uint8	lock_delay_units;		// 1 = ms 2 = decpcmsampl
	uint16	lock_delay;				// time for endp to lock internal
									//		clock recovery circuitry
} _PACKED usb_as_cs_endpoint_descriptor;



/* 3-byte integer */
typedef struct {
	uint8	data[3];
} _PACKED usb_triplet;

// and

/*
 * Audio data formats spec
 */

// Table 2-2 and 2-3, page 10
typedef struct { // TODO: optimize!!!
	uint8	lower_sam_freq[3];
	uint8	upper_sam_freq[3];
} _PACKED usb_audio_continuous_freq_descr;

typedef struct {
	uint8	sam_freq[1][3];
} _PACKED usb_audio_discrete_freq_descr;

typedef union {
	usb_audio_continuous_freq_descr cont;
	usb_audio_discrete_freq_descr   discr;
} _PACKED usb_audio_sam_freq_descr;

// Table 2-1, page 10
typedef struct {
	uint8 length;						// 0e for
	uint8 descriptor_type;				// UAS_CS_INTERFACE (0x24)
	uint8 descriptor_subtype;			// UAS_FORMAT_TYPE (0x02)
	uint8 format_type;					// UAF_FORMAT_TYPE_I (0x01)
		struct {
			uint8 nr_channels;					// hopefully 2
			uint8 subframe_size;				// 1, 2, or 4 bytes
			uint8 bit_resolution;				// 8, 16 or 20 bits
			uint8 sam_freq_type;				// 0 == continuous, 1 == a fixed
												//		number of discrete sam freqs
			usb_audio_sam_freq_descr	sf;		// union
		//	uint8 sam_freq[12 * 3];
		} _PACKED typeI;

		struct {
			uint16 max_bit_rate;				// max bit rate in kbits/sec
			uint16 samples_per_frame;			// samples per frame
			uint8 sam_freq_type;				// 0 == continuous, 1 == a fixed
												//		number of discrete sam freqs
			usb_audio_sam_freq_descr	sf;		// union
		//	uint8 sam_freq[12 * 3];
		} _PACKED typeII;

		struct {
			uint8 nr_channels;					// hopefully 2
			uint8 subframe_size;				// 1, 2, or 4 bytes
			uint8 bit_resolution;				// 8, 16 or 20 bits
			uint8 sam_freq_type;				// 0 == continuous, 1 == a fixed
												//		number of discrete sam freqs
			usb_audio_sam_freq_descr	sf;		// union
		//	uint8 sam_freq[12 * 3];
		} _PACKED typeIII;

//} _PACKED usb_type_I_format_descriptor;
} _PACKED usb_format_descriptor;

// Table 2-4, page 13
/*typedef struct {
	uint8 length;						// 0e for
	uint8 descriptor_type;				// UAS_CS_INTERFACE (0x24)
	uint8 descriptor_subtype;			// UAS_FORMAT_TYPE (0x02)
	uint8 format_type;					// UAF_FORMAT_TYPE_II (0x02)
	uint16 max_bit_rate;				// max bit rate in kbits/sec
	uint16 samples_per_frame;			// samples per frame
	uint8 sam_freq_type;				// 0 == continuous, 1 == a fixed
										//		number of discrete sam freqs
	usb_audio_sam_freq_descr	sf;		// union
//	uint8 sam_freq[12 * 3];
} _PACKED usb_type_II_format_descriptor;

// Table 2-23, page 26  (the same as Type I)
typedef struct {
	uint8 length;						// 0e for
	uint8 descriptor_type;				// UAS_CS_INTERFACE (0x24)
	uint8 descriptor_subtype;			// UAS_FORMAT_TYPE (0x02)
	uint8 format_type;					// UAF_FORMAT_TYPE_III (0x03)
	uint8 nr_channels;					// hopefully 2
	uint8 subframe_size;				// 1, 2, or 4 bytes
	uint8 bit_resolution;				// 8, 16 or 20 bits
	uint8 sam_freq_type;				// 0 == continuous, 1 == a fixed
										//		number of discrete sam freqs
	usb_audio_sam_freq_descr	sf;		// union
//	uint8 sam_freq[12 * 3];
} _PACKED usb_type_III_format_descriptor;*/

#ifdef __cplusplus
}
#endif

#endif
