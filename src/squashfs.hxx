/**
 * SquashFS delta tools
 * (c) 2014 Michał Górny
 * Released under the terms of the 2-clause BSD license
 */

#pragma once
#ifndef SDT_SQUASHFS_HXX
#define SDT_SQUASHFS_HXX 1

/**
 * Partial description of SquashFS structures.
 * based upon fs/squashfs/squashfs_fs.h from Linux sources (GPL2+)
 * Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008
 * Phillip Lougher <phillip@squashfs.org.uk>
 */

extern "C"
{
#	include <stdint.h>
}

#include "compressor.hxx"
#include "util.hxx"

namespace squashfs
{
	// Note: everything is little-endian

	const uint32_t magic = 0x73717368UL;
	const uint32_t invalid_frag = 0xffffffffUL;

	const int metadata_size = 8192;

	// compression algos
	namespace compression
	{
		enum compression
		{
			zlib = 1,
			lzma = 2,
			lzo = 3,
			xz = 4
		};
	}

	// bit fields
	namespace inode_size
	{
		const uint16_t uncompressed = 1 << 15;
	}

	namespace block_size
	{
		const uint32_t uncompressed = 1 << 24;
	}

#	pragma pack(push, 1)

	struct super_block
	{
		le32 s_magic;
		le32 inodes;
		le32 mkfs_time;
		le32 block_size;
		le32 fragments;
		le16 compression;
		le16 block_log;
		le16 flags;
		le16 no_ids;
		le16 s_major;
		le16 s_minor;
		le64 root_inode;
		le64 bytes_used;
		le64 id_table_start;
		le64 xattr_id_table_start;
		le64 inode_table_start;
		le64 directory_table_start;
		le64 fragment_table_start;
		le64 lookup_table_start;
	};

	struct dir_index
	{
		le32 index;
		le32 start_block;
		le32 size;

		//unsigned char name[0];
		unsigned char* name();
	};

	namespace inode
	{
		// inode types
		namespace type
		{
			enum type
			{
				dir = 1,
				reg = 2,
				symlink = 3,
				blkdev = 4,
				chrdev = 5,
				fifo = 6,
				socket = 7,
				ldir = 8,
				lreg = 9,
				lsymlink = 10,
				lblkdev = 11,
				lchrdev = 12,
				lfifo = 13,
				lsocket = 14
			};
		}

		struct base
		{
			le16 inode_type;
			le16 mode;
			le16 uid;
			le16 guid;
			le32 mtime;
			le32 inode_number;
		};

		struct ipc : public base
		{
			le32 nlink;
		};

		struct lipc : public ipc
		{
			le32 xattr;
		};

		struct dev : public ipc
		{
			le32 rdev;
		};

		struct ldev : public dev
		{
			le32 xattr;
		};

		struct symlink : public ipc
		{
			le32 symlink_size;

			//char symlink[0];
			char* symlink_name();

			size_t inode_size();
		};

		struct reg : public base
		{
			le32 start_block;
			le32 fragment;
			le32 offset;
			le32 file_size;

			//le32 block_list[0];
			le32* block_list();

			uint32_t block_count(uint32_t block_size, uint16_t block_log);
			size_t inode_size(uint32_t block_size, uint16_t block_log);
		};

		struct lreg : public base
		{
			le64 start_block;
			le64 file_size;
			le64 sparse;
			le32 nlink;
			le32 fragment;
			le32 offset;
			le32 xattr;

			//le32 block_list[0];
			le32* block_list();

			size_t inode_size(uint32_t block_size, uint16_t block_log);
		};

		struct dir : public base
		{
			le32 start_block;
			le32 nlink;
			le16 file_size;
			le16 offset;
			le32 parent_inode;
		};

		struct ldir : public ipc
		{
			le32 file_size;
			le32 start_block;
			le32 parent_inode;
			le16 i_count;
			le16 offset;
			le32 xattr;

			//struct squashfs_dir_index   index[0];
			struct dir_index* index();
		};

		union inode
		{
			base as_base;
			ipc as_ipc;
			lipc as_lipc;
			dev as_dev;
			ldev as_ldev;
			symlink as_symlink;
			reg as_reg;
			lreg as_lreg;
			dir as_dir;
			ldir as_ldir;
		};
	}

#	pragma pack(pop)
}

class MetadataBlockReader
{
	MMAPFile f;
	const Compressor& compressor;

public:
	MetadataBlockReader(const MMAPFile& new_file,
			size_t offset, const Compressor& c);

	size_t read(void* dest, size_t dest_size);
};

class MetadataReader
{
	MetadataBlockReader f;

	char buf[2 * squashfs::metadata_size];
	char* bufp;
	size_t buf_filled;

	void poll_data();

public:
	size_t block_num;

	MetadataReader(const MMAPFile& new_file,
			size_t offset, const Compressor& c);

	void* peek(size_t length);
	void seek(size_t length);
};

class InodeReader
{
	MetadataReader f;

	uint32_t inode_num;
	uint32_t no_inodes;
	uint32_t block_size;
	uint16_t block_log;

public:
	InodeReader(const MMAPFile& new_file,
		const struct squashfs::super_block& sb,
		const Compressor& c);

	union squashfs::inode::inode& read();

	size_t block_num();
};

#endif /*!SDT_SQUASHFS_HXX*/
