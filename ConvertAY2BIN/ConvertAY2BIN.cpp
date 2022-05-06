#include <iostream>
#include <vector>

#define PACK_REGS 1

int Write(std::vector<uint8_t>& outputRegisters, const int numRegsInSet);
void PackValidBits(uint8_t* regs, std::vector<uint8_t>& packedRegisters);
int DoWriteFile(uint8_t* pData, size_t length, const int chunkNumber);

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

	for (int i = 0; i < registers.size(); i += numRegsInSet)
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

#if PACK_REGS
	std::vector<uint8_t> packedRegisters;
	for (int i = 0; i < registers.size(); i += numRegsInSet)
	{
		PackValidBits(&registers[i], packedRegisters);
	}

	const int numPackedRegsInSet = 8;
	int writeRes = Write(packedRegisters, numPackedRegsInSet);
#else
	int writeRes = Write(registers, numRegsInSet);
#endif

	return writeRes;
}

int Write(std::vector<uint8_t>&outputRegisters, const int numRegsInSet)
{
#if 1
	// Use size of 0x2000-2 to get an even number of chunks (-2 for 0xff,0xff end-marker)
	// . 5 chunks for packed regs
	// . 7 chunks for unpacked regs
	const uint32_t chunkSize = (0x2000-2) - ((0x2000-2) % numRegsInSet);	// we want a complete set of registers
#else
	// Use size of 0x1C00-2 to get an even number of chunks (-2 for 0xff,0xff end-marker)
	const uint32_t chunkSize = (0x1C00-2) - ((0x1C00-2) % numRegsInSet);	// we want a complete set of registers
#endif
	int chunkNumber = 0;
	uint8_t* pData = outputRegisters.data();
	size_t lengthRemaining = outputRegisters.size();

	while (lengthRemaining)
	{
		const size_t length = (lengthRemaining > chunkSize) ? chunkSize : lengthRemaining;
		int res = DoWriteFile(pData, length, chunkNumber);
		if (res)
			return res;

		pData += length;
		lengthRemaining -= length;
		chunkNumber++;

#if 1
		// Check if last chunk will result in odd number of chunks
		// - if so, then output 2 smaller equal-size chunks
		if (lengthRemaining < chunkSize && ((chunkNumber & 1) == 0))
		{
			const size_t length2 = (((lengthRemaining / 2) + (numRegsInSet - 1)) / numRegsInSet) * numRegsInSet;	// integral AY set
			res = DoWriteFile(pData, length2, chunkNumber);
			if (res)
				return res;

			pData += length2;
			chunkNumber++;

			res = DoWriteFile(pData, lengthRemaining-length2, chunkNumber);
			if (res)
				return res;

			break;
		}
#endif
	}

	return 0;
}

#define FILENAME "bearagrudge-AY%d.bin"

int DoWriteFile(uint8_t* pData, size_t length, const int chunkNumber)
{
	char filename[100];
	sprintf_s(filename, sizeof(filename), FILENAME, chunkNumber);

	FILE* fh2;
	errno_t res2 = fopen_s(&fh2, filename, "wb");
	if (res2 || !fh2)
		return 1;

	size_t w_count = fwrite(pData, 1, length, fh2);
	if (length != w_count)
		return 1;

	uint8_t endMarker[2] = { 0xff,0xff };
	w_count = fwrite(endMarker, 1, sizeof(endMarker), fh2);
	if (sizeof(endMarker) != w_count)
		return 1;

	fclose(fh2);
	return 0;
}

/*
AY_AFINE   = 0; b7:0
AY_ACOARSE = 1; b3:0
AY_BFINE   = 2; b7:0
AY_BCOARSE = 3; b3:0
AY_CFINE   = 4; b7:0
AY_CCOARSE = 5; b3:0
AY_NOISEPER= 6; b4:0
AY_ENABLE  = 7; b7:0 - for AY-3-8913, there is no PORTA / B so b7:6 are redundant
AY_AVOL    = 8; b4:0 - b4 is envelope which isn't used
AY_BVOL    = 9; b4:0 - b4 is envelope which isn't used
AY_CVOL    =10; b4:0 - b4 is envelope which isn't used

8 + 4 + 8 + 4 + 8 + 4 + 5 + 6 + 4 + 4 + 4 = 59 bits = > 7.5 bytes
5025 lines * 7.5 bytes = 37,687.5

8 + 8 + 8 + (4 + 4) + (4 + 4) + (4 + 4) + 5 + 6
5025 lines * 8 bytes = 40,200
*/
void PackValidBits(uint8_t* regs, std::vector<uint8_t>& packedRegisters)
{
	packedRegisters.push_back(regs[0]);
	packedRegisters.push_back(regs[2]);
	packedRegisters.push_back(regs[4]);
	packedRegisters.push_back((regs[1] << 4) | (regs[3] & 0xf));
	packedRegisters.push_back((regs[5] << 4) | (regs[8] & 0xf));
	packedRegisters.push_back((regs[9] << 4) | (regs[10] & 0xf));
	packedRegisters.push_back(regs[6] & 0x1f);	// 5 bits (3 unused)
	packedRegisters.push_back(regs[7] & 0x3f);	// 6 bits (2 unused)
	_ASSERT((regs[8] & 0x10) == 0);
	_ASSERT((regs[9] & 0x10) == 0);
	_ASSERT((regs[10] & 0x10) == 0);
}
