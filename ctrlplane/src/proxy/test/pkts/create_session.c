U8  tpkt[] = {
    0x48,0x20,0x01,0x1a,0x00,0x00,0x00,0x00,0x17,0x6d,0x05,0x00,
    0x01,0x00,0x08,0x00,0x24,0x60,0x97,0x70,    0x77,0x77,0x77,0xf7,    // imsi(240679077777777)
    0x03,0x00,0x01,0x00,0x03,                                           // recovery

    0x47,0x00,0x1d,0x00,0x05,0x72,0x61,0x74,0x65,0x6c,0x03,0x63,0x6f,0x6d,0x06,0x6d,
    0x6e,0x63,0x30,0x31,0x30,0x06,0x6d,0x63,0x63,0x34,0x34,0x30,0x04,0x67,0x70,0x72,
    0x73,                                                               // apn

    0x48,0x00,0x08,0x00,0x00,0x41,0x89,0x37,0x00,0x41,0x89,0x37,        // ambr
    0x4b,0x00,0x08,0x00,0x53,0x72,0x22,0x70,0x96,0x74,0x16,0x10,        // mei
    0x4c,0x00,0x06,0x00,0x88,0x88,0x88,0x88,0x88,0x88,                  // msisdn => 888888888888
    0x4d,0x00,0x04,0x00,0x00,0x00,0x00,0x00,                            // indication

    0x4e,0x00,0x3a,0x00,0x80,0x80,0x21,0x10,0x01,0x00,0x00,0x10,0x81,0x06,0x00,0x00,
    0x00,0x00,0x83,0x06,0x00,0x00,0x00,0x00,0x00,0x0d,0x00,0x00,0x0a,0x00,0x00,0x05,
    0x00,0xc0,0x23,0x1a,0x01,0x00,0x00,0x1a,0x0f,0x72,0x61,0x74,0x65,0x6c,0x40,0x72,
    0x61,0x74,0x65,0x6c,0x2e,0x63,0x6f,0x6d,0x05,0x72,0x61,0x74,0x65,0x6c,
    // pco
    0x4f,0x00,0x05,0x00,0x01,0x00,0x00,0x00,0x00,                       // paa
    0x52,0x00,0x01,0x00,0x06,                                           // rat
    0x53,0x00,0x03,0x00,0x42,0xf0,0x01,                                 // saving network
    0x56,0x00,0x0d,0x00,0x18,0x42,0xf0,0x01,0x14,0x0c,0x42,0xf0,0x01,0x02,0x15,0x2c,
    0x00,                                                               // user location info
    0x57,0x00,0x09,0x00,0x86,0x00,0x12,0xf5,0x5a,0x31,0x67,0x0a,0x34,   // fteid
    0x5d,0x00,0x2c,0x00,0x49,0x00,0x01,0x00,0x05,0x50,0x00,0x16,0x00,0x7c,0x09,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x57,0x00,0x09,0x02,0x84,0x40,0x76,0x41,0x20,0x31,0x67,0x0a,0x0c,
    // bearer context
    0x63,0x00,0x01,0x00,0x01,                                            // pdn type
    0x7f,0x00,0x01,0x00,0x00,                                            // apn restriction
    0x80,0x00,0x01,0x00,0x01,                                            // selection mode
    0x72,0x00,0x02,0x00,0x63,0x00                                        // ue time zone
};

U8 real_req[] = {
        0x48,0x20,0x01,0x4e,0x00,0x00,0x00,0x00,
        0x12,0xfe,0xcc,0x00,

        // 0x01,0x00,0x08,0x00, 0x42,0x60,0x37,0x41,0x29,0x06,0x71,0xf9, // IMSI
        0x01,0x00,0x08,0x00, 0x42,0x60,0x97,0x70,0x77,0x77,0x77,0xf7,    // imsi(240679077777777)

        0x03,0x00,0x01,0x00,0x05,0x47,0x00,0x1d,
        0x00,0x05,0x72,0x61,0x74,0x65,0x6c,0x03,
        0x63,0x6f,0x6d,0x06,0x6d,0x6e,0x63,0x30,
        0x31,0x30,0x06,0x6d,0x63,0x63,0x34,0x34,
        0x30,0x04,0x67,0x70,0x72,0x73,0x48,0x00,
        0x08,0x00,0x00,0x41,0x89,0x37,0x00,0x41,
        0x89,0x37,0x4b,0x00,0x08,0x00,0x53,0x69,
        0x44,0x60,0x53,0x25,0x37,0x52,

        0x4c,0x00,0x06,0x00,0x18,0x08,0x62,0x57,0x53,0x45, // MSISDN

        0x4d,0x00,0x04,0x00,0x00,0x00,0x00,0x00,
        0x4e,0x00,0x6e,0x00,0x80,0xc2,0x23,0x24,
        0x01,0x00,0x00,0x24,0x10,0x3f,0x87,0xc6,
        0xc6,0x3f,0x87,0xc6,0xc6,0x3f,0x87,0xc6,
        0xc6,0x3f,0x87,0xc6,0xc6,0x72,0x61,0x74,
        0x65,0x6c,0x40,0x72,0x61,0x74,0x65,0x6c,
        0x2e,0x63,0x69,0x6d,0xc2,0x23,0x24,0x02,
        0x00,0x00,0x24,0x10,0x08,0x45,0x07,0x85,
        0x05,0x4d,0x8f,0xba,0x20,0x4d,0x26,0x98,
        0x1b,0x14,0x11,0xa0,0x72,0x61,0x74,0x65,
        0x6c,0x40,0x72,0x61,0x74,0x65,0x6c,0x2e,
        0x63,0x69,0x6d,0x80,0x21,0x10,0x01,0x00,
        0x00,0x10,0x81,0x06,0x00,0x00,0x00,0x00,
        0x83,0x06,0x00,0x00,0x00,0x00,0x00,0x0d,
        0x00,0x00,0x0a,0x00,0x00,0x05,0x00,0x00,
        0x10,0x00,0x4f,0x00,0x05,0x00,0x01,0x00,
        0x00,0x00,0x00,0x52,0x00,0x01,0x00,0x06,
        0x53,0x00,0x03,0x00,0x42,0xf0,0x01,0x56,
        0x00,0x0d,0x00,0x18,0x42,0xf0,0x01,0x14,
        0xf4,0x42,0xf0,0x01,0x02,0xc5,0xa3,0x51,
        0x57,0x00,0x09,0x00,0x86,0x00,0x0d,0x3f,
        0xd1,

        0x7f,0x00,0x00,0x07, // SGW GTP-C IPv4 127.0.0.7

        0x5d,0x00,0x2c,
        0x00,0x49,0x00,0x01,0x00,0x0b,0x50,0x00,
        0x16,0x00,0x7c,0x09,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x57,0x00,0x09,0x02,0x84,0x40,0xa9,0x7f,
        0xff,0x31,0x67,0x06,0xcd,0x63,0x00,0x01,
        0x00,0x01,0x7f,0x00,0x01,0x00,0x00,0x80,
        0x00,0x01,0x00,0x01,0x72,0x00,0x02,0x00,
        0x63,0x00
};


U8 real_res[] = {

        0x48,0x21,0x00,0xcc,0x00,0x0d,0x3f,0xd1,
        0x12,0xfe,0xcc,0x00,0x02,0x00,0x02,0x00,
        0x10,0x00,0x57,0x00,0x09,0x01,0x87,0xb0,
        0x00,0x2b,0x0f,0x6e,0x2c,0xb5,0x12,0x4f,
        0x00,0x05,0x00,0x01,0x0a,0x00,0x2b,0x0f,
        0x7f,0x00,0x01,0x00,0x00,0x48,0x00,0x08,
        0x00,0x00,0x41,0x89,0x37,0x00,0x41,0x89,
        0x37,0x4e,0x00,0x27,0x00,0x80,0x00,0x0d,
        0x04,0x08,0x08,0x08,0x08,0x00,0x0d,0x04,
        0x08,0x08,0x04,0x04,0x00,0x10,0x02,0x05,
        0xdc,0x80,0x21,0x10,0x03,0x00,0x00,0x10,
        0x81,0x06,0x08,0x08,0x08,0x08,0x83,0x06,
        0x08,0x08,0x04,0x04,0x5d,0x00,0x43,0x00,
        0x02,0x00,0x02,0x00,0x10,0x00,0x49,0x00,
        0x01,0x00,0x0b,0x57,0x00,0x09,0x02,0x85,
        0xb0,0x00,0x2b,0x0f,0x6e,0x2c,0xb5,0xa0,
        0x4f,0x00,0x05,0x00,0x01,0x0a,0x00,0x2b,
        0x0f,0x50,0x00,0x16,0x00,0x04,0x09,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x5e,0x00,0x04,0x00,0xb0,
        0x00,0x2b,0x0f,0x03,0x00,0x01,0x00,0x01,
        0xff,0x00,0x1c,0x00,0xc7,0x69,0x00,0x00,
        0xaf,0xbe,0xad,0xde,0x6e,0x2c,0xb5,0xa0,
        0x40,0xa9,0x7f,0xff,0x00,0x00,0x2b,0x0f,
        0x0f,0x2b,0x00,0xb0,0x00,0x00,0x00,0x00
};

