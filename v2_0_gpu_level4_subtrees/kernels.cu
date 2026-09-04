// kernel.cu
// Copyright (c) 2026 Sam Hawker <smhwkr@googlemail.com>. All rights reserved.

#include <stdio.h>

#include <cuda.h>
#include <cuda_runtime.h>

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

__device__ __forceinline__ uint32_t ptx_lanemask_lt()
{
    uint32_t ret;
    asm("mov.u32 %0, %lanemask_lt;" : "=r"(ret));
    return ret;
}

__device__ __forceinline__ int ptx_bfind_u32(uint32_t i)
{
    int ret;
    asm("bfind.u32 %0, %1;" : "=r"(ret) : "r"(i));
    return ret;
}

__device__ __forceinline__ int find_first_unset(uint64_t i)
{
    uint32_t lo = i & 0xffffffff;
    uint32_t hi = i >> 32;
    uint32_t bitlo = (lo + 1) & ~lo;
    uint32_t bithi = (hi + 1) & ~hi;
    bool uselo = (lo != 0xffffffff);
    uint32_t bit = uselo ? bitlo : bithi;
    int ret = ptx_bfind_u32(bit);
    if (!uselo)
    {
        ret += 32;
    }
    return ret;
}

__global__ void __launch_bounds__(1024, 1) kernelPartialSolve(const ulonglong1* __restrict__ occupiedfromid, int maxpartials, int* __restrict__ partials, ushort1* __restrict__ partialpieces, ulonglong1* __restrict__ partialoccupied, ushort4* __restrict__ partialids)
{
    int piece0 = blockIdx.x;
    int pieces0 = 1 << piece0;
    int piece1 = blockIdx.y + ((blockIdx.y >= blockIdx.x) ? 1 : 0);
    int pieces1 = 1 << piece1;
    pieces1 |= pieces0;
    int piece2 = blockIdx.z;
    int pieces2 = 1 << piece2;
    if (!(pieces1 & pieces2))
    {
        pieces2 |= pieces1;
        int placement0 = threadIdx.x;
        int target0 = 0;
        int id0 = (target0 << 9) + (piece0 << 5) + placement0;
        uint64_t occupied0 = occupiedfromid[id0].x;
        if (occupied0)
        {
            int placement1 = threadIdx.y;
            int target1 = find_first_unset(occupied0);
            int id1 = (target1 << 9) + (piece1 << 5) + placement1;
            uint64_t occupied1 = occupiedfromid[id1].x;
            if (occupied1 && !(occupied0 & occupied1))
            {
                occupied1 |= occupied0;
                int target2 = find_first_unset(occupied1);
                int id2 = (target2 << 9) + (piece2 << 5);
                while (1)
                {
                    uint64_t occupied2 = occupiedfromid[id2].x;
                    if (!occupied2)
                    {
                        break;
                    }
                    if (!(occupied1 & occupied2))
                    {
                        occupied2 |= occupied1;
                        int target3 = find_first_unset(occupied2);
                        for (int piece3 = 0; piece3 < 13; piece3++)
                        {
                            int pieces3 = 1 << piece3;
                            if (!(pieces2 & pieces3))
                            {
                                pieces3 |= pieces2;
                                int id3 = (target3 << 9) + (piece3 << 5);
                                while (1)
                                {
                                    uint64_t occupied3 = occupiedfromid[id3].x;
                                    if (!occupied3)
                                    {
                                        break;
                                    }
                                    if (!(occupied2 & occupied3))
                                    {
                                        occupied3 |= occupied2;
                                        int slot = atomicAdd(partials, 1);
                                        if (slot < maxpartials)
                                        {
                                            partialpieces[slot] = ushort1({ static_cast<uint16_t>(0x1fff ^ pieces3) });
                                            partialoccupied[slot] = ulonglong1({ occupied3 });
                                            partialids[slot] = ushort4({ static_cast<uint16_t>(id0), static_cast<uint16_t>(id1), static_cast<uint16_t>(id2), static_cast<uint16_t>(id3) });
                                        }
                                    }
                                    id3++;
                                }
                            }
                        }
                    }
                    id2++;
                }
            }
        }
    }
}

extern "C" void cudaPartialSolve(const ulonglong1* occupiedfromid, int maxpartials, int* partials, ushort1* partialpieces, ulonglong1* partialoccupied, ushort4* partialids)
{
    dim3 block_dim(32, 32, 1);
    dim3 grid_dim(13, 12, 13);

    kernelPartialSolve<<<grid_dim, block_dim>>> (occupiedfromid, maxpartials, partials, partialpieces, partialoccupied, partialids);
}

__global__ void __launch_bounds__(128, 12) kernelSolve(const ulonglong1* __restrict__ occupiedfromid, const int* __restrict__ partials, const ushort1* __restrict__ partialpieces, const ulonglong1* __restrict__ partialoccupied, const ushort4* __restrict__ partialids, int* __restrict__ active, int* __restrict__ partial, int maxsolutions, int* __restrict__ solutions, ushort4(* __restrict__ solutionids)[4])
{
    if (!threadIdx.x && !threadIdx.y)
    {
        atomicAdd(active, 128);
    }

    int toplevel = 0;

    uint32_t pieces = 0;
    uint64_t occupied;

    uint16_t id0;
    uint16_t id1;
    uint16_t id2;
    uint16_t id3;
    uint16_t ids[9];

    int level = 0;
    int targetn;
    uint32_t piecenbit;
    int piecen;
    int id;

    int slot = (blockIdx.x << 7) + (threadIdx.y << 5) + threadIdx.x;
    if (slot < *partials)
    {
        // read from the input buffer
        pieces = partialpieces[slot].x;
        occupied = partialoccupied[slot].x;
        id0 = partialids[slot].x;
        id1 = partialids[slot].y;
        id2 = partialids[slot].z;
        id3 = partialids[slot].w;
        targetn = find_first_unset(occupied);
        piecenbit = pieces & -pieces;
        piecen = ptx_bfind_u32(piecenbit);
        id = (targetn << 9) + (piecen << 5);

        while (1)
        {
            uint64_t occupiedn = occupiedfromid[id].x;

            if (occupied & occupiedn)
            {
                // conflicts
                // the null terminator cannot cause a conflict by definition
                // threads which find a conflict get another chance to find something more substantial
                id++;
                occupiedn = occupiedfromid[id].x;
            }

            if (occupied & occupiedn)
            {
                // conflicts
                // the null terminator cannot cause a conflict by definition
                id++;
                continue;
            }

            if (occupiedn)
            {
                // fits
                ids[level] = static_cast<uint16_t>(id);
                if (level + 1 == 9)
                {
                    // found a solution
                    int slot = atomicAdd(solutions, 1);
                    if (slot < maxsolutions)
                    {
                        solutionids[slot][0] = ushort4({ id0, id1, id2, id3 });
                        solutionids[slot][1] = ushort4({ ids[0], ids[1], ids[2], ids[3] });
                        solutionids[slot][2] = ushort4({ ids[4], ids[5], ids[6], ids[7] });
                        solutionids[slot][3] = ushort4({ ids[8], static_cast<uint16_t>(0), static_cast<uint16_t>(0), static_cast<uint16_t>(0) });
                    }
                    id++;
                    continue;
                }
                // descend one level
                level++;
                pieces ^= piecenbit;
                occupied |= occupiedn;
                targetn = find_first_unset(occupied);
                piecenbit = pieces & -pieces;
                piecen = ptx_bfind_u32(piecenbit);
                id = (targetn << 9) + (piecen << 5);
                continue;
            }
            // select a piece that has not already been used
            uint32_t temppieces = pieces & -(piecenbit << 1);
            if (temppieces)
            {
                // found a piece
                piecenbit = temppieces & -temppieces;
                piecen = ptx_bfind_u32(piecenbit);
                id = (targetn << 9) + (piecen << 5);
                continue;
            }
            // all pieces are invalid
            if (level > toplevel)
            {
                // ascend one level
                level--;
                id = ids[level];
                targetn = id >> 9;
                piecen = (id >> 5) & 15;
                piecenbit = 1U << piecen;
                occupiedn = occupiedfromid[id].x;
                pieces ^= piecenbit;
                occupied &= ~occupiedn;
                id++;
                continue;
            }
            // exhausted search space
            break;
        }
    }

    __syncthreads();

    // nothing left to do
    if (!threadIdx.x && !threadIdx.y)
    {
        atomicAdd(active, -128);
    }
}

extern "C" void cudaSolve(const ulonglong1* occupiedfromid, const int* partials, const ushort1* partialpieces, const ulonglong1* partialoccupied, const ushort4* partialids, int* active, int* partial, int maxsolutions, int* solutions, ushort4(*solutionids)[4])
{
    dim3 block_dim(32, 4, 1);
    dim3 grid_dim(32768, 1, 1);

    *active = 0;

    cudaFuncSetCacheConfig(kernelSolve, cudaFuncCachePreferL1);

    kernelSolve<<<grid_dim, block_dim>>> (occupiedfromid, partials, partialpieces, partialoccupied, partialids, active, partial, maxsolutions, solutions, solutionids);
}
