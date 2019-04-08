// Generated Macro Name:	MakeLoopbackMeasurement
// Generated Time: 		Sep-11-2018 10:23:04
// Paste this whole file into an interface to transfer Macro functionality.
{
	uint8_t* addrs = new uint8_t[universalAddressSize_];  // create address buffer
	                                                      // of interface size
	uint8_t* data =
	    new uint8_t[universalDataSize_];  // create data buffer of interface size

	// universalWrite(0x91a0,0x00000000);
	{
		uint8_t macroAddrs[2] = {0xa0, 0x91};  // create macro address buffer
		for(unsigned int i = 0; i < universalAddressSize_;
		    ++i)  // fill with macro address and 0 fill
			addrs[i] = (i < 2) ? macroAddrs[i] : 0;

		uint8_t macroData[4] = {0x00, 0x00, 0x00, 0x00};  // create macro data buffer
		for(unsigned int i = 0; i < universalDataSize_;
		    ++i)  // fill with macro address and 0 fill
			data[i] = (i < 4) ? macroData[i] : 0;
		universalWrite((char*)addrs, (char*)data);
	}

	// universalWrite(0x9154,0x00000000);
	{
		uint8_t macroAddrs[2] = {0x54, 0x91};  // create macro address buffer
		for(unsigned int i = 0; i < universalAddressSize_;
		    ++i)  // fill with macro address and 0 fill
			addrs[i] = (i < 2) ? macroAddrs[i] : 0;

		uint8_t macroData[4] = {0x00, 0x00, 0x00, 0x00};  // create macro data buffer
		for(unsigned int i = 0; i < universalDataSize_;
		    ++i)  // fill with macro address and 0 fill
			data[i] = (i < 4) ? macroData[i] : 0;
		universalWrite((char*)addrs, (char*)data);
	}

	// universalWrite(0x9114,0x00000000);
	{
		uint8_t macroAddrs[2] = {0x14, 0x91};  // create macro address buffer
		for(unsigned int i = 0; i < universalAddressSize_;
		    ++i)  // fill with macro address and 0 fill
			addrs[i] = (i < 2) ? macroAddrs[i] : 0;

		uint8_t macroData[4] = {0x00, 0x00, 0x00, 0x00};  // create macro data buffer
		for(unsigned int i = 0; i < universalDataSize_;
		    ++i)  // fill with macro address and 0 fill
			data[i] = (i < 4) ? macroData[i] : 0;
		universalWrite((char*)addrs, (char*)data);
	}

	// universalWrite(0x9114,0x00000101);
	{
		uint8_t macroAddrs[2] = {0x14, 0x91};  // create macro address buffer
		for(unsigned int i = 0; i < universalAddressSize_;
		    ++i)  // fill with macro address and 0 fill
			addrs[i] = (i < 2) ? macroAddrs[i] : 0;

		uint8_t macroData[4] = {0x01, 0x01, 0x00, 0x00};  // create macro data buffer
		for(unsigned int i = 0; i < universalDataSize_;
		    ++i)  // fill with macro address and 0 fill
			data[i] = (i < 4) ? macroData[i] : 0;
		universalWrite((char*)addrs, (char*)data);
	}

	// universalWrite(0x9380,0x00000101);
	{
		uint8_t macroAddrs[2] = {0x80, 0x93};  // create macro address buffer
		for(unsigned int i = 0; i < universalAddressSize_;
		    ++i)  // fill with macro address and 0 fill
			addrs[i] = (i < 2) ? macroAddrs[i] : 0;

		uint8_t macroData[4] = {0x01, 0x01, 0x00, 0x00};  // create macro data buffer
		for(unsigned int i = 0; i < universalDataSize_;
		    ++i)  // fill with macro address and 0 fill
			data[i] = (i < 4) ? macroData[i] : 0;
		universalWrite((char*)addrs, (char*)data);
	}

	// delay(10);
	sleep(10);

	// universalRead(0x9360,data);
	{
		uint8_t macroAddrs[2] = {0x60, 0x93};  // create macro address buffer
		for(unsigned int i = 0; i < universalAddressSize_;
		    ++i)  // fill with macro address and 0 fill
			addrs[i] = (i < 2) ? macroAddrs[i] : 0;

		universalRead((char*)addrs, (char*)data);
	}

	// universalWrite(0x9380,0x00000000);
	{
		uint8_t macroAddrs[2] = {0x80, 0x93};  // create macro address buffer
		for(unsigned int i = 0; i < universalAddressSize_;
		    ++i)  // fill with macro address and 0 fill
			addrs[i] = (i < 2) ? macroAddrs[i] : 0;

		uint8_t macroData[4] = {0x00, 0x00, 0x00, 0x00};  // create macro data buffer
		for(unsigned int i = 0; i < universalDataSize_;
		    ++i)  // fill with macro address and 0 fill
			data[i] = (i < 4) ? macroData[i] : 0;
		universalWrite((char*)addrs, (char*)data);
	}
	delete[] addrs;  // free the memory
	delete[] data;   // free the memory
}