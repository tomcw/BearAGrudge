#include <iostream>
#include <vector>

int main()
{
	const int numRegsInSet = 11;

	FILE* fh1;
	errno_t res1 = fopen_s(&fh1, "bearagrudge-AY-log(via-FUSE)-reduced.txt", "rt");
	if (res1 || !fh1)
		return 1;

	std::vector<uint8_t> registers;

	bool eof = false;
	while (!eof)
	{
		uint32_t reg[numRegsInSet];
		for (int i = 0; i < numRegsInSet; i++)
		{
			int res = fscanf_s(fh1, "%x", &reg[i]);
			if (res != 1)
			{
				eof = true;
				break;
			}
		}

		if (eof)
			break;

		for (int i = 0; i < numRegsInSet; i++)
			registers.push_back(reg[i]);
	}

	fclose(fh1);

	// Convert ABC periods from ZX Spectrum128 to AppleII
	// . ZX Spectrum's CLK for AY8912 = 1.77345MHz
	// . PAL:14.25045e6 / 14          = 1.01789MHz
	// . NTSC:14.3181818e6 / 14       = 1.02273MHz

	for (int i = 0; i < registers.size(); i+=numRegsInSet)
	{
		for (int channel = 0; channel < 3; channel++)
		{
			int oldPeriod = registers[i + channel * 2] + registers[i + channel * 2 + 1] * 256;
			int newPeriod = (int)((float)oldPeriod * 1.02273 / 1.77345);
			registers[i + channel * 2] = newPeriod & 0xff;
			registers[i + channel * 2 + 1] = newPeriod / 256;
		}
	}

	//

#define FILENAME "bearagrudge-AY%d.bin"

	// Use size of 0x1C00 to get an even number of chunks (size of 0x2000 gives us 7)
	const uint32_t chunkSize = 0x1C00 - (0x1C00 % numRegsInSet);	// we want a complete set of registers
	int chunkNumber = 0;
	uint8_t* pData = registers.data();
	size_t lengthRemaining = registers.size();

	while (lengthRemaining)
	{
		char filename[100];
		sprintf_s(filename, sizeof(filename), FILENAME, chunkNumber);

		FILE* fh2;
		errno_t res2 = fopen_s(&fh2, filename, "wb");
		if (res1 || !fh2)
			return 1;

		const size_t length = (lengthRemaining > chunkSize) ? chunkSize : lengthRemaining;
		size_t w_count = fwrite(pData, 1, length, fh2);
		if (length != w_count)
			return 1;

		uint8_t endMarker[2] = { 0xff,0xff };
		w_count = fwrite(endMarker, 1, sizeof(endMarker), fh2);
		if (sizeof(endMarker) != w_count)
			return 1;

		fclose(fh2);

		pData += length;
		lengthRemaining -= length;
		chunkNumber++;
	}

	return 0;
}
