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
#include <list>

#include <cstdio>
#include <cstring>

#include "compressor.hxx"
#include "squashfs.hxx"
#include "util.hxx"

struct compressed_block
{
	size_t offset;
	size_t length;
	// XXX: checksum
};

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

			std::list<struct compressed_block> compressed_blocks;

			std::cerr << "Input: " << input_file << "\n";
			std::cerr << "Reading inodes..." << std::endl;

			InodeReader ir(f, sb, *c);

			for (uint32_t i = 0; i < sb.inodes; ++i)
			{
				union squashfs::inode::inode& in = ir.read();

				if (in.as_base.inode_type == squashfs::inode::type::reg)
				{
					uint32_t pos = in.as_reg.start_block;
					le32* block_list = in.as_reg.block_list();

					for (uint32_t j = 0;
							j < in.as_reg.block_count(sb.block_size, sb.block_log);
							++j)
					{
						if (block_list[j] & squashfs::block_size::uncompressed)
						{
							// seek over the uncompressed block
							pos += (block_list[j] & ~squashfs::block_size::uncompressed);
						}
						else
						{
							// record the compressed block
							struct compressed_block block;
							block.offset = pos;
							block.length = block_list[j];

							compressed_blocks.push_back(block);
							pos += block.length;
						}
					}
				}
			}

			std::cerr << "Read " << sb.inodes << " inodes in "
				<< ir.block_num() << " blocks.\n";

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
