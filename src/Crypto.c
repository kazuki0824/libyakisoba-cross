#include <string.h>
#include "Global.h"
#include "Crypto.h"

//

static const u8 Sbox[32] = {
	0x46, 0xA2, 0x72, 0x41, 0xE2, 0x8C, 0xDF, 0x11,
	0x81, 0x7D, 0x18, 0x44, 0xB1, 0x0D, 0xD7, 0x42,
	0xBE, 0x76, 0x6D, 0x81, 0xAF, 0x7A, 0xD2, 0x05,
	0x76, 0x84, 0xFD, 0x2A, 0x62, 0x0D, 0x76, 0xA1,
};

static const u8 LookupTable[256] = {
	0xAA, 0xA2, 0x10, 0xFA, 0xA9, 0xF0, 0x40, 0x2F, 0xB1, 0x1C, 0x1A, 0x6F, 0x43, 0xB4, 0x73, 0xBC,
	0x69, 0x77, 0xC5, 0x00, 0xF3, 0xD4, 0x09, 0x7E, 0x58, 0x8D, 0x44, 0xC3, 0xF5, 0x54, 0x0C, 0xDD,
	0x3F, 0xB7, 0xD1, 0xD6, 0x9A, 0xD3, 0x39, 0x82, 0x01, 0x5E, 0x03, 0xED, 0x78, 0x63, 0x90, 0x49,
	0x9B, 0x15, 0xA8, 0x4F, 0x67, 0x52, 0xAC, 0xE4, 0x37, 0xEA, 0xF7, 0x23, 0x55, 0x0F, 0x42, 0x12,
	0xE3, 0x05, 0x5F, 0x2D, 0x2E, 0x7F, 0x11, 0x38, 0x07, 0xF4, 0x3C, 0xE2, 0xD5, 0x9F, 0xDF, 0xCF,
	0x30, 0x0B, 0xAD, 0x66, 0x22, 0x70, 0xEF, 0x7B, 0xA6, 0x24, 0x65, 0x0D, 0x5D, 0x79, 0x02, 0x4D,
	0x0E, 0x32, 0x84, 0x97, 0xB8, 0x57, 0x34, 0xE8, 0x41, 0x87, 0xC1, 0xF9, 0x9C, 0x56, 0xAE, 0x71,
	0xAB, 0xBF, 0xD0, 0x88, 0x25, 0xC8, 0x1F, 0xD7, 0xFE, 0x04, 0x4E, 0xCE, 0x51, 0x81, 0xBB, 0xCD,
	0x91, 0xA5, 0x14, 0x75, 0xA4, 0x60, 0x61, 0x6E, 0x7A, 0xE6, 0x99, 0xD8, 0xA0, 0x4C, 0xDC, 0x1B,
	0x06, 0x6C, 0x3E, 0x9E, 0xF8, 0xCB, 0x98, 0x92, 0x0A, 0xFB, 0x2A, 0xCA, 0x50, 0x7C, 0xC0, 0x83,
	0x94, 0xB5, 0x6A, 0x21, 0x95, 0xB3, 0x48, 0xD9, 0x16, 0xA7, 0xEE, 0x4B, 0xFD, 0x9D, 0xBD, 0x6B,
	0xC6, 0x80, 0x20, 0x3A, 0x53, 0x1E, 0x5C, 0xC7, 0xB6, 0x08, 0xAF, 0xA1, 0x2B, 0x19, 0x26, 0x8A,
	0x47, 0xE1, 0x86, 0x74, 0xE9, 0x59, 0x62, 0x8B, 0x28, 0x6D, 0xEC, 0x76, 0xB0, 0x45, 0xC2, 0x46,
	0x4A, 0xE0, 0xF2, 0x8C, 0xBE, 0x3B, 0x5B, 0xBA, 0x31, 0x96, 0xE5, 0x36, 0x8E, 0xEB, 0xE7, 0xB9,
	0xA3, 0x35, 0x17, 0x68, 0x27, 0x8F, 0x85, 0x89, 0x29, 0x93, 0xFF, 0xFC, 0xDE, 0x7D, 0x18, 0xDB,
	0x64, 0xF6, 0x1D, 0xB2, 0x3D, 0xF1, 0xC9, 0x13, 0xDA, 0xCC, 0xC4, 0x72, 0x33, 0x5A, 0xD2, 0x2C,
};

static const u8 InitialisationVector1[8] = { 0x32, 0x96, 0x57, 0xE8, 0x7B, 0x02, 0x4C, 0xD3 };
static const u8 InitialisationVector2[8] = { 0x70, 0x39, 0xCD, 0x1A, 0x46, 0x52, 0xE8, 0xFB };

//

typedef struct {
	u32 Word;
} Word_t;

//

static u32 LoadLE32(const u8 *p)
{
	return ((u32)p[0] << 0) |
	       ((u32)p[1] << 8) |
	       ((u32)p[2] << 16) |
	       ((u32)p[3] << 24);
}

static u32 LoadPartialLE32(const u8 *p, u32 Size)
{
	u32 Value = 0;

	for (u32 i = 0; i < Size; i++)
		Value |= (u32)p[i] << (i * 8);

	return Value;
}

static void StoreLE32(u8 *p, u32 Value)
{
	p[0] = (u8)(Value >> 0);
	p[1] = (u8)(Value >> 8);
	p[2] = (u8)(Value >> 16);
	p[3] = (u8)(Value >> 24);
}

static u64 LoadLE64(const u8 *p)
{
	return ((u64)LoadLE32(p)) | ((u64)LoadLE32(p + 4) << 32);
}

static u64 LoadWordsLE64(const Word_t *Words)
{
	return ((u64)Words[0].Word) | ((u64)Words[1].Word << 32);
}

static void StoreWordsLE64(Word_t *Words, u64 Value)
{
	Words[0].Word = (u32)Value;
	Words[1].Word = (u32)(Value >> 32);
}

static u32 BSwap32(u32 Value)
{
	return ((Value & 0x000000ff) << 24) |
	       ((Value & 0x0000ff00) << 8) |
	       ((Value & 0x00ff0000) >> 8) |
	       ((Value & 0xff000000) >> 24);
}

static u8 WordByte(u32 Word, unsigned Index)
{
	return (u8)(Word >> (Index * 8));
}

static u32 SetWordByte(u32 Word, unsigned Index, u8 Value)
{
	u32 Shift = (u32)Index * 8;
	u32 Mask = (u32)0xff << Shift;

	return (Word & ~Mask) | ((u32)Value << Shift);
}

static u16 WordBE16Lane(u32 Word, unsigned Lane)
{
	return (u16)(((u16)WordByte(Word, Lane * 2) << 8) |
	             ((u16)WordByte(Word, Lane * 2 + 1)));
}

static u32 SetWordBE16Lane(u32 Word, unsigned Lane, u16 Value)
{
	Word = SetWordByte(Word, Lane * 2, (u8)(Value >> 8));
	Word = SetWordByte(Word, Lane * 2 + 1, (u8)Value);
	return Word;
}

static u32 SwapNibbles(u32 W)
{
	return ((W & 0xf0f0f0f0) >> 4) | ((W & 0x0f0f0f0f) << 4);
}

static u32 RotateLeft(u32 Value)
{
	Value = BSwap32(Value);
	Value = (Value << 1) | (Value >> 31);

	return BSwap32(Value);
}

static u8 RotateByteLeft(u8 Value)
{
	Value = (Value << 1) | (Value >> 7);

	return Value;
}

static int IsOddParity(u8 Input)
{
	int Count = 0;
	for (int i = 0 ; i < 8 ; i++) {
		if (Input & 1)
			Count++;
		Input >>= 1;
	}

	return (Count & 1) ? TRUE : FALSE;
}

static u8 GetBox(u8 Round, u8 WorkKey)
{
	u8 Box = Sbox[((Round & 0x0c) >> 2) + ((WorkKey & 0xe0) >> 3)];
	u8 i = (0xff ^ Round) & 3;
	while (i-- > 0)
		Box >>= 2;

	return Box;
}

static void Scramble(Word_t *Tmp)
{
	u8 b0, b1, b2, b3;

	Tmp[0].Word ^= Tmp[1].Word;

	b0 = LookupTable[WordByte(Tmp[0].Word, 0)];
	b1 = LookupTable[WordByte(Tmp[0].Word, 1)];
	b2 = LookupTable[WordByte(Tmp[0].Word, 2)];
	b3 = LookupTable[WordByte(Tmp[0].Word, 3)];
	b1 ^= RotateByteLeft(b0);
	b0 ^= RotateByteLeft(b1);
	b3 ^= RotateByteLeft(b2);
	b2 ^= RotateByteLeft(b3);
	b1 ^= RotateByteLeft(RotateByteLeft(b2));
	b2 ^= RotateByteLeft(RotateByteLeft(b1));
	b3 ^= RotateByteLeft(RotateByteLeft(b0));
	b0 ^= RotateByteLeft(RotateByteLeft(b3));

	Tmp[0].Word = ((u32)b0 << 0) |
	              ((u32)b1 << 8) |
	              ((u32)b2 << 16) |
	              ((u32)b3 << 24);
}

static void BlockCipher(Word_t *Cipher, Word_t Pass, u8 Box)
{
	u8 Counter = (Box & 2) ? 0x53 : 0;
	u16 CC = (Counter << 8) | Counter;
	u32 T;

	Cipher->Word = SetWordBE16Lane(Cipher->Word, 0,
					  (u16)(WordBE16Lane(Cipher->Word, 0) + WordBE16Lane(Pass.Word, 0) + CC));
	Cipher->Word = SetWordBE16Lane(Cipher->Word, 1,
					  (u16)(WordBE16Lane(Cipher->Word, 1) + WordBE16Lane(Pass.Word, 1) + CC));
	Cipher->Word = SwapNibbles(Cipher->Word);

	u32 Rot = RotateLeft(Pass.Word);
	u32 Mask = Cipher->Word & Rot;
	u8 x = (u8)((Mask >> 24) ^ (Mask >> 16) ^ (Mask >> 8) ^ Mask);

	if (IsOddParity(x) == TRUE) {
		Cipher->Word ^= 0xffffffff ^ Rot;
	}

	T = BSwap32(Cipher->Word);

	if ((Box & 1) == 0) {
		T =	((T & 0x00000055) <<  1) | ((T & 0x0000aa00) <<  7) |
			((T & 0x00550000) <<  9) | ((T & 0x000000aa) << 16) |

			((T & 0x55000000) >>  0) | ((T & 0x00aa5500) >>  8) | ((T & 0xaa000000) >> 17);
	} else {
		T =	((T & 0x00aa0000) <<  7) | ((T & 0x00005500) <<  9) |
			((T & 0x00000055) << 16) | ((T & 0xaa000000) >>  0) |

			((T & 0x000000aa) >>  1) | ((T & 0x0055aa00) >>  8) | ((T & 0x55000000) >> 15);
	}

	Cipher->Word = BSwap32(T);
	Cipher->Word ^= BSwap32((T << 8) | (T >> 24)) ^ RotateLeft(Cipher->Word);
}

static void ProcessBlockCipher0x(u8 Protocol, Word_t *Cipher, const u8 *Key, int DecryptionMode)
{
	u8 Start, End, Step;

	(void)Protocol;

	if (DecryptionMode == TRUE) {
		Start = 0x0f;
		End = 0xff;
		Step = 0xff;
	} else {
		Start = 0x00;
		End = 0x10;
		Step = 0x01;
	}

	u32 Copy;
	Word_t Pass;
		
	for (u8 Round = Start ; Round != End ; Round += Step) {
		Copy = Cipher[1].Word;
		Cipher[1].Word = Cipher[0].Word;
		Cipher[0].Word = Copy;

		u8 Box = GetBox(Round, DecryptionMode);

		Pass.Word = LoadLE32(&Key[(Round & 3) << 2]);
		BlockCipher(&Cipher[0], Pass, Box);

		Cipher[1].Word ^= Cipher[0].Word;
		Cipher[0].Word = Copy;
	}

	Copy = Cipher[0].Word;
	Cipher[0].Word = Cipher[1].Word;
	Cipher[1].Word = Copy;
}

void ProcessBlockCipher4x(u8 Protocol, Word_t *Cipher, const u8 *Key, int DecryptionMode)
{
	const u8 *IV;
	u64 E;

	if (Protocol & 0x0c)
		IV = InitialisationVector2;
	else
		IV = InitialisationVector1;

	E = LoadWordsLE64(Cipher);
	E += LoadLE64(IV);
	StoreWordsLE64(Cipher, E);

	u8 Index, Step;
	if (DecryptionMode == TRUE) {
		Index = 0x0f;
		Step = 0xff;
	} else {
		Index = 0x00;
		Step = 0x01;
	}
	for (u8 i = 0 ; i < 16 ; i++) {
		Word_t Tmp[2];
		Tmp[0].Word = Cipher[1].Word;
		Tmp[1].Word = LoadLE32(Key + ((u32)Index * 4));
		Index += Step;
		Scramble(Tmp);
		Cipher[0].Word ^= Tmp[0].Word;
		if (i != 15) {
			u32 x;
			x = Cipher[1].Word;
			Cipher[1].Word = Cipher[0].Word;
			Cipher[0].Word = x;
		}
	}

	E = LoadWordsLE64(Cipher);
	E -= LoadLE64(IV);
	StoreWordsLE64(Cipher, E);
}

static void SetupKeySchedule0x(u8 Protocol, const u8 *InKey, u8 *Schedule)
{
	u8 IV[2][4] = {
		{ 0x6a, 0xa3, 0x2b, 0x6f },
		{ 0x84, 0xe5, 0xc4, 0xe7 },
	};

	u8 Key[16];
	Word_t Pass;

	memcpy(Key, InKey, 8);

	for (int i = 8 ; i < 16 ; i++)
		Key[i] = i;

	if (Protocol & 0x0c)
		Pass.Word = LoadLE32(IV[1]);
	else
		Pass.Word = LoadLE32(IV[0]);

	for (int i = 0 ; i < 8 ; i++) {
		Word_t Cipher;

		Cipher.Word = LoadLE32(Key + ((i & 3) << 2));
		BlockCipher(&Cipher, Pass, 0);
		Pass.Word = Cipher.Word;
		StoreLE32(Key + ((i & 3) << 2), Cipher.Word);
	}

	memcpy(Schedule, Key, 16);
}

static void SetupKeySchedule4x(u8 Protocol, const u8 *InKey, u8 *Schedule)
{
	u8 Key[8], x;

	(void)Protocol;
	memcpy(Key, InKey, 8);

	for (u8 i = 0, j = 0 ; i < 16 ; i++, j += 4) {
		Schedule[j + 0] = (Key[0] += LookupTable[Key[1]]);
		Schedule[j + 1] = (Key[2] ^= LookupTable[Key[3]]);
		Schedule[j + 2] = (Key[4] ^= LookupTable[Key[5]]);
		Schedule[j + 3] = (Key[6] += LookupTable[Key[7]]);
		x = Key[0];
		Key[0] = Key[1];
		Key[1] = Key[2];
		Key[2] = Key[3];
		Key[3] = Key[4];
		Key[4] = Key[5];
		Key[5] = Key[6];
		Key[6] = Key[7];
		Key[7] = x;
	}
}

void Transform(u8 Protocol, const u8 *Key, const u8 *Input, u32 Size, u8 *Output, int Decryption)
{
	u32 AlignedSize = Size / 8;
	u32 Remainder = Size - (AlignedSize * 8);

	const u8 IV[8] = { 0xfe, 0x27, 0x19, 0x99, 0x19, 0x69, 0x09, 0x11 };
	u32 Previous[2];

	u8 Schedule128[16];
	u8 Schedule512[64];

	union {
		Word_t Cipher[2];
		u8 CipherBytes[8];
	} u;

	Previous[0] = LoadLE32(IV);
	Previous[1] = LoadLE32(IV + 4);

	if (Protocol & 0x40)
		SetupKeySchedule4x(Protocol, Key, Schedule512);
	else
		SetupKeySchedule0x(Protocol, Key, Schedule128);

	for ( ; AlignedSize > 0 ; AlignedSize--) {
		u32 InputWords[2];

		InputWords[0] = LoadLE32(Input);
		InputWords[1] = LoadLE32(Input + 4);
		u.Cipher[0].Word = InputWords[0];
		u.Cipher[1].Word = InputWords[1];

		if (Decryption == FALSE) {
			u.Cipher[0].Word ^= Previous[0];
			u.Cipher[1].Word ^= Previous[1];
		}

		if (Protocol & 0x40)
			ProcessBlockCipher4x(Protocol, u.Cipher, Schedule512, Decryption);
		else
			ProcessBlockCipher0x(Protocol, u.Cipher, Schedule128, Decryption);

		if (Decryption == TRUE) {
			u.Cipher[0].Word ^= Previous[0];
			u.Cipher[1].Word ^= Previous[1];
		}

		StoreLE32(Output, u.Cipher[0].Word);
		StoreLE32(Output + 4, u.Cipher[1].Word);

		if (Decryption == FALSE) {
			Previous[0] = u.Cipher[0].Word;
			Previous[1] = u.Cipher[1].Word;
		} else {
			Previous[0] = InputWords[0];
			Previous[1] = InputWords[1];
		}

		Input += 8;
		Output += 8;
	}

	if (Remainder > 0) {
		u.Cipher[0].Word = Previous[0];
		u.Cipher[1].Word = Previous[1];

		if (Protocol & 0x40)
			ProcessBlockCipher4x(Protocol, u.Cipher, Schedule512, FALSE);
		else
			ProcessBlockCipher0x(Protocol, u.Cipher, Schedule128, FALSE);

		StoreLE32(u.CipherBytes, u.Cipher[0].Word);
		StoreLE32(u.CipherBytes + 4, u.Cipher[1].Word);
		for (u32 i = 0 ; i < Remainder ; i++)
			u.CipherBytes[i] ^= Input[i];

		memcpy(Output, u.CipherBytes, Remainder);
	}
}

static void ProcessMAC0x(Word_t *Plain, const u32 *Seed)
{
	u32 Copy;
	Word_t Pass;

	Copy = Plain[1].Word;
	Plain[1].Word = Plain[0].Word;
	Plain[0].Word = Copy;

	Pass.Word = Seed[0];
	BlockCipher(&Plain[0], Pass, 3);
	Plain[1].Word ^= Plain[0].Word;
	Plain[0].Word = Copy;

	Copy = Plain[1].Word;
	Plain[1].Word = Plain[0].Word;
	Plain[0].Word = Copy;

	Pass.Word = Seed[1];
	BlockCipher(&Plain[0], Pass, 3);
	Plain[1].Word ^= Plain[0].Word;
	Plain[0].Word = Copy;
}

// make this lib re-entrant
//static Word_t Counter;

static void sub_B1D4(Word_t *Tmp, Word_t *Counter)
{
	Counter->Word++;
	Scramble(Tmp);
	Tmp[1].Word += Tmp[0].Word;
}

static void GenerateMAC4x(u8 Protocol, const u8 *Key, const u8 *Payload, u32 Size, u8 *MAC)
{
	u32 AlignedSize = Size / 4;
	u32 UnalignedSize = Size - (AlignedSize * 4);
	Word_t Tmp[2];
	const u8 *IV;
	Word_t Counter;


	if (Protocol & 0x0c)
		IV = InitialisationVector2;
	else
		IV = InitialisationVector1;

	Counter.Word = 0;
	Tmp[1].Word = LoadLE32(IV);
	Tmp[0].Word = LoadLE32(Key);
	sub_B1D4(Tmp, &Counter);

	for ( ; AlignedSize > 0 ; AlignedSize--) {
		Tmp[0].Word = LoadLE32(Payload);
		sub_B1D4(Tmp, &Counter);
		Payload += 4;
	}

	switch (Size & 7) {
	case 0:
		break;
	case 1:
	case 2:
	case 3:
		Tmp[0].Word = LoadPartialLE32(Payload, UnalignedSize);
		sub_B1D4(Tmp, &Counter);
		// Fallthrough
	case 4:
		Tmp[0].Word = 0;
		sub_B1D4(Tmp, &Counter);
		break;
	default:
		if (UnalignedSize == 0)
			break;
		Tmp[0].Word = LoadPartialLE32(Payload, UnalignedSize);
		sub_B1D4(Tmp, &Counter);
		break;
	}

	Tmp[0].Word = LoadLE32(IV + 4);
	Tmp[0].Word += LoadLE32(Key + 4);
	Tmp[0].Word += Counter.Word;

	Tmp[0].Word++;
	sub_B1D4(Tmp, &Counter);

	StoreLE32(MAC, Tmp[1].Word);
}

static void GenerateMAC0x(const u8 *Key, const u8 *Payload, u32 Size, u8 *MAC)
{
	u32 AlignedSize = Size / 8;
	u32 UnalignedSize = Size - (AlignedSize * 8);
	u32 IV[2];
	u32 Seed[2];

	Seed[0] = LoadLE32(Key);
	Seed[1] = LoadLE32(Key + 4);

	memset(&IV, 0, sizeof(IV));

	Word_t Plain[2] = { { 0 }, { 0 } };

	for ( ; AlignedSize > 0 ; AlignedSize--) {
		Plain[0].Word = LoadLE32(Payload);
		Plain[1].Word = LoadLE32(Payload + 4);
		Plain[0].Word ^= IV[0];
		Plain[1].Word ^= IV[1];

		ProcessMAC0x(Plain, Seed);
		IV[0] = Plain[0].Word;
		IV[1] = Plain[1].Word;

		Payload += 8;
	}

	if (UnalignedSize > 0) {
		Plain[0].Word = 0;
		Plain[1].Word = 0;
		if (UnalignedSize <= 4) {
			Plain[0].Word = LoadPartialLE32(Payload, UnalignedSize);
		} else {
			Plain[0].Word = LoadLE32(Payload);
			Plain[1].Word = LoadPartialLE32(Payload + 4, UnalignedSize - 4);
		}
		Plain[0].Word ^= IV[0];
		Plain[1].Word ^= IV[1];
		ProcessMAC0x(Plain, Seed);
	}

	StoreLE32(MAC, Plain[1].Word);
}

void GenerateMAC(u8 Protocol, const u8 *Key, const u8 *Payload, u32 Size, u8 *MAC)
{
	if (Protocol & 0x40)
		GenerateMAC4x(Protocol, Key, Payload, Size, MAC);
	else
		GenerateMAC0x(Key, Payload, Size, MAC);
}
