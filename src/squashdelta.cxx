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
#include <typeinfo>

#include <cstdio>
#include <cstring>

#include "compressor.hxx"
#include "hash.hxx"
#include "squashfs.hxx"
#include "util.hxx"

struct compressed_block
{
	size_t offset;
	size_t length;
	uint32_t hash;
};

bool sort_by_offset(const struct compressed_block& lhs,
		const struct compressed_block& rhs)
{
	return lhs.offset < rhs.offset;
}

bool sort_by_len_hash(const struct compressed_block& lhs,
		const struct compressed_block& rhs)
{
	if (lhs.length == rhs.length)
		return lhs.hash < rhs.hash;
	return lhs.length < rhs.length;
}


std::list<struct compressed_block> get_blocks(MMAPFile& f, Compressor*& c)
{
	const squashfs::super_block& sb = f.peek<squashfs::super_block>();

	if (sb.s_magic != squashfs::magic)
		throw std::runtime_error(
				"File is not a valid SquashFS image (no magic).");
	if (sb.s_major != 4 || sb.s_minor != 0)
		throw std::runtime_error("File is not SquashFS 4.0");

	switch (sb.compression)
	{
		case squashfs::compression::lzo:
#ifdef ENABLE_LZO
			if (!c)
				c = new LZOCompressor();
			else if (typeid(*c) != typeid(LZOCompressor))
				throw std::runtime_error("The two files use different compressors");
#else
			throw std::runtime_error("LZO compression support disabled at build time");
#endif
			break;
		default:
			throw std::runtime_error("Unsupported compression algorithm.");
	}

	std::list<struct compressed_block>
		compressed_metadata_blocks,
		compressed_data_blocks;

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

					compressed_data_blocks.push_back(block);
					pos += block.length;
				}
			}
		}
	}

	size_t block_num = ir.block_num();
	std::cerr << "Read " << sb.inodes << " inodes in "
		<< block_num << " blocks.\n";

	// record inode blocks

	std::cerr << "Hashing " << block_num
		<< " inode blocks..." << std::endl;

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
			block.hash = murmurhash3(data, length, 0);

			compressed_metadata_blocks.push_back(block);
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

			compressed_data_blocks.push_back(block);
		}
	}

	block_num = fr.block_num();
	std::cerr << "Read " << sb.fragments << " fragments in "
		<< block_num << " blocks.\n";

	// record fragment table

	std::cerr << "Hashing " << block_num
		<< " fragment table blocks..." << std::endl;

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
			block.hash = murmurhash3(data, length, 0);

			compressed_metadata_blocks.push_back(block);
		}
	}

	// sort by offset to use sequential reads
	compressed_data_blocks.sort(sort_by_offset);

	std::cerr << "Hashing " << compressed_data_blocks.size()
		<< " data blocks..." << std::endl;
	MMAPFile hf(f);

	// record the checksums
	for (std::list<struct compressed_block>::iterator
			i = compressed_data_blocks.begin();
			i != compressed_data_blocks.end();
			++i)
	{
		hf.seek((*i).offset, std::ios::beg);
		(*i).hash = murmurhash3(hf.read_array<uint8_t>((*i).length),
				(*i).length, 0);
	}

	compressed_data_blocks.splice(compressed_data_blocks.end(),
			compressed_metadata_blocks);

	std::cerr << "Total: " << compressed_data_blocks.size()
		<< " compressed blocks." << std::endl;

	return compressed_data_blocks;
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

	MMAPFile source_f, target_f;

	std::list<struct compressed_block> source_blocks;
	std::list<struct compressed_block> target_blocks;

	Compressor* c = 0;

	try
	{
		source_f.open(source_file);
		std::cerr << "Source: " << source_file << "\n";
		source_blocks = get_blocks(source_f, c);
	}
	catch (IOError& e)
	{
		std::cerr << "Program terminated abnormally:\n\t"
			<< e.what() << "\n\tat file: " << source_file
			<< "\n\terrno: " << strerror(e.errno_val) << "\n";
		if (c)
			delete c;
		return 1;
	}
	catch (std::exception& e)
	{
		std::cerr << "Program terminated abnormally:\n\t"
			<< e.what() << "\n\tat file: " << source_file << "\n";
		if (c)
			delete c;
		return 1;
	}

	std::cerr << "\n";

	try
	{
		target_f.open(target_file);
		std::cerr << "Target: " << target_file << "\n";
		target_blocks = get_blocks(target_f, c);
	}
	catch (IOError& e)
	{
		std::cerr << "Program terminated abnormally:\n\t"
			<< e.what() << "\n\tat file: " << source_file
			<< "\n\terrno: " << strerror(e.errno_val) << "\n";
		delete c;
		return 1;
	}
	catch (std::exception& e)
	{
		std::cerr << "Program terminated abnormally:\n\t"
			<< e.what() << "\n\tat file: " << source_file << "\n";
		delete c;
		return 1;
	}

	std::cerr << "\n";

	source_blocks.sort(sort_by_len_hash);
	target_blocks.sort(sort_by_len_hash);

	for (std::list<struct compressed_block>::iterator
			i = source_blocks.begin(),
			j = target_blocks.begin();
			i != source_blocks.end() && j != target_blocks.end();)
	{
		// seek until we find duplicates
		if ((*i).length < (*j).length)
			++i;
		else if ((*j).length < (*i).length)
			++j;
		else if ((*i).hash < (*j).hash)
			++i;
		else if ((*j).hash < (*i).hash)
			++j;
		else
		{
			// found a match, remove the blocks then
			std::list<struct compressed_block>::iterator
				i_st = i, j_st = j;

			// remove consecutive duplicates as well
			while (i != source_blocks.end()
					&& (*i).length == (*i_st).length
					&& (*i).hash == (*i_st).hash)
				++i;
			while (j != target_blocks.end()
					&& (*j).length == (*j_st).length
					&& (*j).hash == (*j_st).hash)
				++j;

			source_blocks.erase(i_st, i);
			target_blocks.erase(j_st, j);
		}
	}

	std::cerr << "Unique blocks found: "
		<< source_blocks.size() << " in source and "
		<< target_blocks.size() << " in target.\n";

	delete c;

	return 0;
}
