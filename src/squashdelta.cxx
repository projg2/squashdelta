/**
 * SquashFS delta tools
 * (c) 2014 Michał Górny
 * Released under the terms of the 2-clause BSD license
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include <iostream>
#include <iomanip>
#include <cstdio>
#include <cstring>

#include "compressor.hxx"
#include "squashfs.hxx"
#include "util.hxx"

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		std::cerr << "Usage: " << argv[0] << " <input-file>\n";
		return 1;
	}

	const char* input_file = argv[1];

	try
	{
		MMAPFile f;
		f.open(input_file);

		const squashfs::super_block& sb = f.peek<squashfs::super_block>();

		if (sb.s_magic != squashfs::magic)
		{
			std::cerr << "File is not a valid SquashFS image (no magic).\n";
			return 1;
		}
		if (sb.s_major != 4 || sb.s_minor != 0)
		{
			std::cerr << "SquashFS version " << sb.s_major << "."
				<< sb.s_minor << " found while only 4.0 is supported.\n";
			return 1;
		}

		Compressor* c;

		try
		{
			switch (sb.compression)
			{
				case squashfs::compression::lzo:
					c = new LZOCompressor();
					break;
				default:
					std::cerr << "Unsupported compression algorithm.\n";
					return 1;
			}

			InodeReader ir(f, sb, *c);

			for (uint32_t i = 0; i < sb.inodes; ++i)
				ir.read();

			delete c;
		}
		catch (std::exception& e)
		{
			delete c;
			throw;
		}
	}
	catch (IOError& e)
	{
		std::cerr << "Program terminated abnormally:\n\t"
			<< e.what() << "\n\tat file: " << input_file
			<< "\n\terrno: " << strerror(e.errno_val) << "\n";
		return 1;
	}
	catch (std::exception& e)
	{
		std::cerr << "Program terminated abnormally:\n\t"
			<< e.what() << "\n";
		return 1;
	}

	return 0;
}
