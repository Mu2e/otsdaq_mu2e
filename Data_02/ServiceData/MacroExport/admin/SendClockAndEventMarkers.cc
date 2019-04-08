// Generated Macro Name:	SendClockAndEventMarkers
// Generated Time: 		Sep-10-2018 03:40:43
// Paste this whole file into an interface to transfer Macro functionality.
{
	uint8_t addrs[universalAddressSize_];  // create address buffer of interface size
	uint8_t data[universalDataSize_];      // create data buffer of interface size

	// universalWrite(0x91a8,0x0000ffff);
	{
		uint8_t macroAddrs[2] = {0xa8, 0x91};  // create macro address buffer
		for(unsigned int i = 0; i < universalAddressSize_;
		    ++i)  // fill with macro address and 0 fill
			addrs[i] = (i < 2) ? macroAddrs[i] : 0;

		uint8_t macroData[4] = {0xff, 0xff, 0x00, 0x00};  // create macro data buffer
		for(unsigned int i = 0; i < universalDataSize_;
		    ++i)  // fill with macro address and 0 fill
			data[i] = (i < 4) ? macroData[i] : 0;
		universalWrite((char*)addrs, (char*)data);
	}

	// universalRead(0x91a8,data);
	{
		uint8_t macroAddrs[2] = {0xa8, 0x91};  // create macro address buffer
		for(unsigned int i = 0; i < universalAddressSize_;
		    ++i)  // fill with macro address and 0 fill
			addrs[i] = (i < 2) ? macroAddrs[i] : 0;

		universalRead((char*)addrs, (char*)data);
	}

	// universalWrite(0x9118,0xff0000);
	{
		uint8_t macroAddrs[2] = {0x18, 0x91};  // create macro address buffer
		for(unsigned int i = 0; i < universalAddressSize_;
		    ++i)  // fill with macro address and 0 fill
			addrs[i] = (i < 2) ? macroAddrs[i] : 0;

		uint8_t macroData[3] = {0x00, 0x00, 0xff};  // create macro data buffer
		for(unsigned int i = 0; i < universalDataSize_;
		    ++i)  // fill with macro address and 0 fill
			data[i] = (i < 3) ? macroData[i] : 0;
		universalWrite((char*)addrs, (char*)data);
	}

	// universalWrite(0x9118,0x0);
	{
		uint8_t macroAddrs[2] = {0x18, 0x91};  // create macro address buffer
		for(unsigned int i = 0; i < universalAddressSize_;
		    ++i)  // fill with macro address and 0 fill
			addrs[i] = (i < 2) ? macroAddrs[i] : 0;

		uint8_t macroData[1] = {0x00};  // create macro data buffer
		for(unsigned int i = 0; i < universalDataSize_;
		    ++i)  // fill with macro address and 0 fill
			data[i] = (i < 1) ? macroData[i] : 0;
		universalWrite((char*)addrs, (char*)data);
	}

	// universalWrite(0x9100,0x5);
	{
		uint8_t macroAddrs[2] = {0x00, 0x91};  // create macro address buffer
		for(unsigned int i = 0; i < universalAddressSize_;
		    ++i)  // fill with macro address and 0 fill
			addrs[i] = (i < 2) ? macroAddrs[i] : 0;

		uint8_t macroData[1] = {0x05};  // create macro data buffer
		for(unsigned int i = 0; i < universalDataSize_;
		    ++i)  // fill with macro address and 0 fill
			data[i] = (i < 1) ? macroData[i] : 0;
		universalWrite((char*)addrs, (char*)data);
	}

	// universalWrite(0x9114,0x0000ffff);
	{
		uint8_t macroAddrs[2] = {0x14, 0x91};  // create macro address buffer
		for(unsigned int i = 0; i < universalAddressSize_;
		    ++i)  // fill with macro address and 0 fill
			addrs[i] = (i < 2) ? macroAddrs[i] : 0;

		uint8_t macroData[4] = {0xff, 0xff, 0x00, 0x00};  // create macro data buffer
		for(unsigned int i = 0; i < universalDataSize_;
		    ++i)  // fill with macro address and 0 fill
			data[i] = (i < 4) ? macroData[i] : 0;
		universalWrite((char*)addrs, (char*)data);
	}

	// universalWrite(0x9154,0x80);
	{
		uint8_t macroAddrs[2] = {0x54, 0x91};  // create macro address buffer
		for(unsigned int i = 0; i < universalAddressSize_;
		    ++i)  // fill with macro address and 0 fill
			addrs[i] = (i < 2) ? macroAddrs[i] : 0;

		uint8_t macroData[1] = {0x80};  // create macro data buffer
		for(unsigned int i = 0; i < universalDataSize_;
		    ++i)  // fill with macro address and 0 fill
			data[i] = (i < 1) ? macroData[i] : 0;
		universalWrite((char*)addrs, (char*)data);
	}

	// universalWrite(0x91a8,0x0000ffff);
	{
		uint8_t macroAddrs[2] = {0xa8, 0x91};  // create macro address buffer
		for(unsigned int i = 0; i < universalAddressSize_;
		    ++i)  // fill with macro address and 0 fill
			addrs[i] = (i < 2) ? macroAddrs[i] : 0;

		uint8_t macroData[4] = {0xff, 0xff, 0x00, 0x00};  // create macro data buffer
		for(unsigned int i = 0; i < universalDataSize_;
		    ++i)  // fill with macro address and 0 fill
			data[i] = (i < 4) ? macroData[i] : 0;
		universalWrite((char*)addrs, (char*)data);
	}

	// universalRead(0x91a8,data);
	{
		uint8_t macroAddrs[2] = {0xa8, 0x91};  // create macro address buffer
		for(unsigned int i = 0; i < universalAddressSize_;
		    ++i)  // fill with macro address and 0 fill
			addrs[i] = (i < 2) ? macroAddrs[i] : 0;

		universalRead((char*)addrs, (char*)data);
	}

	// universalWrite(0x9118,0xff0000);
	{
		uint8_t macroAddrs[2] = {0x18, 0x91};  // create macro address buffer
		for(unsigned int i = 0; i < universalAddressSize_;
		    ++i)  // fill with macro address and 0 fill
			addrs[i] = (i < 2) ? macroAddrs[i] : 0;

		uint8_t macroData[3] = {0x00, 0x00, 0xff};  // create macro data buffer
		for(unsigned int i = 0; i < universalDataSize_;
		    ++i)  // fill with macro address and 0 fill
			data[i] = (i < 3) ? macroData[i] : 0;
		universalWrite((char*)addrs, (char*)data);
	}

	// universalWrite(0x9118,0x0);
	{
		uint8_t macroAddrs[2] = {0x18, 0x91};  // create macro address buffer
		for(unsigned int i = 0; i < universalAddressSize_;
		    ++i)  // fill with macro address and 0 fill
			addrs[i] = (i < 2) ? macroAddrs[i] : 0;

		uint8_t macroData[1] = {0x00};  // create macro data buffer
		for(unsigned int i = 0; i < universalDataSize_;
		    ++i)  // fill with macro address and 0 fill
			data[i] = (i < 1) ? macroData[i] : 0;
		universalWrite((char*)addrs, (char*)data);
	}
}