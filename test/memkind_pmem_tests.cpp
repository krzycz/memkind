/*
 * Copyright (C) 2015 - 2017 Intel Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice(s),
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice(s),
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <memkind/internal/memkind_pmem.h>
#include <memkind/internal/memkind_private.h>

#include <sys/param.h>
#include <sys/mman.h>
#include <stdio.h>
#include "common.h"

static const size_t PMEM_PART_SIZE = MEMKIND_PMEM_MIN_SIZE + 4096;
static const char*  PMEM_DIR = "/tmp/";

class MemkindPmemTests: public :: testing::Test
{

protected:
    memkind_t pmem_kind;
    void SetUp()
    {
        // create PMEM partition
        int err = memkind_create_pmem(PMEM_DIR, PMEM_PART_SIZE, &pmem_kind);
        ASSERT_EQ(0, err);
        ASSERT_TRUE(NULL != pmem_kind);
    }

    void TearDown()
    {}
};

static void pmem_get_size(struct memkind *kind, size_t& total, size_t& free)
{
    struct memkind_pmem *priv = reinterpret_cast<struct memkind_pmem *>(kind->priv);

    total = priv->max_size;
    free = priv->max_size - priv->offset; /* rough estimation */
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemPriv)
{
    size_t total_mem = 0;
    size_t free_mem = 0;

    pmem_get_size(pmem_kind, total_mem, free_mem);

    ASSERT_TRUE(total_mem != 0);
    ASSERT_TRUE(free_mem != 0);

    EXPECT_EQ(total_mem, roundup(PMEM_PART_SIZE, MEMKIND_PMEM_CHUNK_SIZE));

    size_t offset = total_mem - free_mem;
    EXPECT_LT(offset, MEMKIND_PMEM_CHUNK_SIZE);
    EXPECT_LT(offset, total_mem);
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemMalloc)
{
    const size_t size = 1024;
    char *default_str = NULL;

    default_str = (char *)memkind_malloc(pmem_kind, size);
    EXPECT_TRUE(NULL != default_str);

    sprintf(default_str, "memkind_malloc MEMKIND_PMEM\n");
    printf("%s", default_str);

    memkind_free(pmem_kind, default_str);

    // Out of memory
    default_str = (char *)memkind_malloc(pmem_kind, 2 * PMEM_PART_SIZE);
    EXPECT_EQ(NULL, default_str);
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemCalloc)
{
    const size_t size = 1024;
    const size_t num = 1;
    char *default_str = NULL;

    default_str = (char *)memkind_calloc(pmem_kind, num, size);
    EXPECT_TRUE(NULL != default_str);
    EXPECT_EQ(*default_str, 0);

    sprintf(default_str, "memkind_calloc MEMKIND_PMEM\n");
    printf("%s", default_str);

    memkind_free(pmem_kind, default_str);

    // allocate the buffer of the same size (likely at the same address)
    default_str = (char *)memkind_calloc(pmem_kind, num, size);
    EXPECT_TRUE(NULL != default_str);
    EXPECT_EQ(*default_str, 0);

    sprintf(default_str, "memkind_calloc MEMKIND_PMEM\n");
    printf("%s", default_str);

    memkind_free(pmem_kind, default_str);
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemCallocHuge)
{
    const size_t size = MEMKIND_PMEM_CHUNK_SIZE;
    const size_t num = 1;
    char *default_str = NULL;

    default_str = (char *)memkind_calloc(pmem_kind, num, size);
    EXPECT_TRUE(NULL != default_str);
    EXPECT_EQ(*default_str, 0);

    sprintf(default_str, "memkind_calloc MEMKIND_PMEM\n");
    printf("%s", default_str);

    memkind_free(pmem_kind, default_str);

    // allocate the buffer of the same size (likely at the same address)
    default_str = (char *)memkind_calloc(pmem_kind, num, size);
    EXPECT_TRUE(NULL != default_str);
    EXPECT_EQ(*default_str, 0);

    sprintf(default_str, "memkind_calloc MEMKIND_PMEM\n");
    printf("%s", default_str);

    memkind_free(pmem_kind, default_str);
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemRealloc)
{
    const size_t size1 = 512;
    const size_t size2 = 1024;
    char *default_str = NULL;

    default_str = (char *)memkind_realloc(pmem_kind, default_str, size1);
    EXPECT_TRUE(NULL != default_str);

    sprintf(default_str, "memkind_realloc MEMKIND_PMEM with size %zu\n", size1);
    printf("%s", default_str);

    default_str = (char *)memkind_realloc(pmem_kind, default_str, size2);
    EXPECT_TRUE(NULL != default_str);

    sprintf(default_str, "memkind_realloc MEMKIND_PMEM with size %zu\n", size2);
    printf("%s", default_str);

    memkind_free(pmem_kind, default_str);
}

//test dziesiec razy alokuje i zwalnia 100 elementowa tablice i do kazdego elementu tej tablicy przypisuje ptr zwracany przez funkcje memkind_posix_memalign
TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemPosixMemalign)
{
	const int max_allocs = 300;
	const int test_value = 123456;
	size_t alignment = 131072;
	unsigned i;
	int *ptrs[10 * max_allocs];
	void *ptr;
	int ret = 0;
	int success = 0;

	for(int j=0; j<10; j++)
	{
		memset(ptrs, 0, sizeof(ptrs));

		for (i = 0; i < max_allocs; ++i)
		{
			errno = 0;
			
#if 1
			ret = memkind_posix_memalign(pmem_kind, &ptr, 65536 /*alignment*/, alignment /*sizeof(void *)*/);
			if (ret != 0)
			{
				std::cout << "Alokacja nie powiodla sie! " << "i= " << i << " j= " << j  << std::endl;
				break;
			}
#else
			ptr = memkind_malloc(pmem_kind, alignment);
			if (ptr == nullptr)
			{
				std::cout << "Alokacja nie powiodla sie! " << "i= " << i << " j= " << j  << std::endl;
				break;
			}
#endif

			EXPECT_EQ(success, ret);
			EXPECT_EQ(errno, 0);

			ptrs[j * max_allocs + i] = (int *)ptr;

			//std::cout << "ptr " << "i:" << i << " ptr: " << ptrs[j * max_allocs + i]  << std::endl;
			std::cout << ptrs[j * max_allocs + i]  << std::endl;

			/* at least one allocation must succeed */
			ASSERT_TRUE(i != 0 || ptr != nullptr);
			if (ptr == nullptr)
				break;

			/* ptr should be usable */
			*(int*)ptr = test_value;
			ASSERT_EQ(*(int*)ptr, test_value);

		}

		for (i = 0; i < max_allocs; ++i) {
			if (ptrs[j * max_allocs + i] == NULL)
				break;
			memkind_free(pmem_kind, ptrs[j * max_allocs + i]);
		}
		std::cout << std::endl;
	}
}

