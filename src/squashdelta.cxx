/**
 * SquashFS delta tools
 * (c) 2014 Michał Górny
 * Released under the terms of the 2-clause BSD license
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include <iostream>
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

std::list<struct compressed_block> get_blocks(const char* path)
{
	MMAPFile f;
	f.open(path);

	const squashfs::super_block& sb = f.peek<squashfs::super_block>();

	if (sb.s_magic != squashfs::magic)
		throw std::runtime_error(
				"File is not a valid SquashFS image (no magic).");
	if (sb.s_major != 4 || sb.s_minor != 0)
		throw std::runtime_error("File is not SquashFS 4.0");

	Compressor* c;

	switch (sb.compression)
	{
		case squashfs::compression::lzo:
#ifdef ENABLE_LZO
			c = new LZOCompressor();
#else
			throw std::runtime_error("LZO compression support disabled at build time");
#endif
			break;
		default:
			throw std::runtime_error("Unsupported compression algorithm.");
	}

	try
	{
		std::list<struct compressed_block> compressed_blocks;

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

		size_t block_num = ir.block_num();
		std::cerr << "Read " << sb.inodes << " inodes in "
			<< block_num << " blocks.\n";

		// record inode blocks

		const char* data_start = static_cast<const char*>(f.data);

		MetadataBlockReader mir(f, sb.inode_table_start, *c);
		for (size_t i = 0; i < block_num; ++i)
		{
			const void* data;
			size_t length;
			bool compressed;

			mir.read_input_block(data, length, compressed);

			if (compressed)
			{
				const char* data_pos = static_cast<const char*>(data);

				struct compressed_block block;
				block.offset = data_pos - data_start;
				block.length = length;

				compressed_blocks.push_back(block);
			}
		}

		// fragments
		std::cerr << "Reading fragment table..." << std::endl;

		FragmentTableReader fr(f, sb, *c);

		for (uint32_t i = 0; i < sb.fragments; ++i)
		{
			struct squashfs::fragment_entry& fe = fr.read();

			if (!(fe.size & squashfs::block_size::uncompressed))
			{
				struct compressed_block block;
				block.offset = fe.start_block;
				block.length = fe.size;

				compressed_blocks.push_back(block);
			}
		}

		block_num = fr.block_num();
		std::cerr << "Read " << sb.fragments << " fragments in "
			<< block_num << " blocks.\n";

		// record fragment table

		MetadataBlockReader mfr(f, fr.start_offset, *c);
		for (size_t i = 0; i < block_num; ++i)
		{
			const void* data;
			size_t length;
			bool compressed;

			mir.read_input_block(data, length, compressed);

			if (compressed)
			{
				const char* data_pos = static_cast<const char*>(data);

				struct compressed_block block;
				block.offset = data_pos - data_start;
				block.length = length;

				compressed_blocks.push_back(block);
			}
		}

		std::cerr << "Total: " << compressed_blocks.size()
			<< " compressed blocks." << std::endl;

		delete c;

		return compressed_blocks;
	}
	catch (std::exception& e)
	{
		delete c;
		throw;
	}
}

int main(int argc, char* argv[])
{
	if (argc < 3)
	{
		std::cerr << "Usage: " << argv[0] << " <source> <target>\n";
		return 1;
	}

	const char* source_file = argv[1];
	const char* target_file = argv[2];

	std::list<struct compressed_block> source_blocks;
	std::list<struct compressed_block> target_blocks;

	try
	{
		std::cerr << "Source: " << source_file << "\n";
		source_blocks = get_blocks(source_file);
	}
	catch (IOError& e)
	{
		std::cerr << "Program terminated abnormally:\n\t"
			<< e.what() << "\n\tat file: " << source_file
			<< "\n\terrno: " << strerror(e.errno_val) << "\n";
		return 1;
	}
	catch (std::exception& e)
	{
		std::cerr << "Program terminated abnormally:\n\t"
			<< e.what() << "\n\tat file: " << source_file << "\n";
		return 1;
	}

	try
	{
		std::cerr << "Target: " << source_file << "\n";
		target_blocks = get_blocks(target_file);
	}
	catch (IOError& e)
	{
		std::cerr << "Program terminated abnormally:\n\t"
			<< e.what() << "\n\tat file: " << source_file
			<< "\n\terrno: " << strerror(e.errno_val) << "\n";
		return 1;
	}
	catch (std::exception& e)
	{
		std::cerr << "Program terminated abnormally:\n\t"
			<< e.what() << "\n\tat file: " << source_file << "\n";
		return 1;
	}

	return 0;
}
