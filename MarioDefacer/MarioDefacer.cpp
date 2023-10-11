#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sys/stat.h>

using namespace std;

int file_exists(char* filename);

enum class Endianness {
	Unknown = -1,
	BigEndian,
	LittleEndian,
	ByteSwapped
};

constexpr int BUFFERSIZE = 1024;

int main(int argc, char** argv)
{
	const uint32_t* noface = new uint32_t[3]{ 0x0c020014, 0x1400020c, 0x020c1400 };
	const char* bup_suffix = " (backup)";
	size_t n_words, pos = 0;
	uint32_t n_faces = 0;
	bool is_first_chunk = true;
	Endianness endianness = Endianness::Unknown;

	if (argc == 1) {
		cout << "Usage: MarioDefacer \"Mario Game 64.z64\"" << endl;
		return -1;
	}

	char* fname = argv[1];
	FILE* filp = fopen(fname, "rb+");
	if (!filp) {
		cout << "Error: could not open file \"" << fname << "\" !" << endl;
		return -1;
	}

	char* bup_fname = new char[strlen(fname) + strlen(bup_suffix) + 1];
	strcpy(bup_fname, fname);
	char* fext = strrchr(fname, '.');

	if (fext) {
		strcpy(bup_fname + strlen(fname) - strlen(fext), bup_suffix);
		strcpy(bup_fname + strlen(fname) - strlen(fext) + strlen(bup_suffix), fext);
	}
	else {
		strcpy(bup_fname + strlen(fname), bup_suffix);
	}

	FILE* backup = nullptr;
	uint32_t* buffer = new uint32_t[BUFFERSIZE];

	while ((n_words = fread(buffer, sizeof(uint32_t), BUFFERSIZE, filp)) > 0) {
		if (is_first_chunk) {
			if (buffer[0] == 0x40123780) {
				endianness = Endianness::BigEndian;
			}
			else if (buffer[0] == 0x80371240) {
				endianness = Endianness::LittleEndian;
			}
			else if (buffer[0] == 0x12408037) {
				endianness = Endianness::ByteSwapped;
			}
			else {
				cout << "Not a valid Nintendo 64 ROM file !" << endl;
				fclose(filp);
				return -1;
			}

			if (!file_exists(bup_fname)) {
				backup = fopen(bup_fname, "w"); // create the output file for writing
				fwrite(buffer, sizeof(uint32_t), n_words, backup);
			}

			is_first_chunk = false;
			continue;
		}

		if (backup) fwrite(buffer, sizeof(uint32_t), n_words, backup);

		for (size_t i = 0; i < n_words; i++) {
			if ((endianness == Endianness::BigEndian && buffer[i] == UINT32_C(0x78000014)) ||
				(endianness == Endianness::LittleEndian && buffer[i] == UINT32_C(0x00147800)) ||
				(endianness == Endianness::ByteSwapped && buffer[i] == UINT32_C(0x00781400))) {
				cout << "Face found at 0x" << hex << setw(8) << setfill('0') << (pos + i) * sizeof(uint32_t) << " !" << endl;
				fseek(filp, (long)(i - n_words) * sizeof(uint32_t), SEEK_CUR);
				fwrite(&noface[(int)endianness], sizeof(uint32_t), 1, filp);
				fseek(filp, (long)(n_words - i - 1) * sizeof(uint32_t), SEEK_CUR);
				n_faces++;
			}
		}

		pos += n_words;
	}

	// Done and close.
	if (backup) fclose(backup);
	fclose(filp);

	if (n_faces) {
		cout << "Removed " << n_faces << " face(s)." << endl;
	}
	else {
		cout << "Found no faces !" << endl;
		remove(bup_fname);
	}

	return 0;
}

int file_exists(char* filename) {
	struct stat buffer;
	return stat(filename, &buffer) == 0;
}
