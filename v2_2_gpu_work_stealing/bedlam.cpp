// bedlam.cpp
// Copyright (c) 2026 Sam Hawker <smhwkr@googlemail.com>. All rights reserved.

#include <windows.h>

#include <stdio.h>

#include <cuda.h>
#include <cuda_runtime.h>

#include <memory>
#include <vector>
#include <set>

extern "C" void cudaPartialSolve(const ulonglong1 * occupiedfromid, int maxpartials, int* partials, ushort1 * partialpieces, ulonglong1 * partialoccupied, ushort4 * partialids);

extern "C" void cudaSolve(const ulonglong1 * occupiedfromid, const int* partials, const ushort1 * partialpieces, const ulonglong1 * partialoccupied, const ushort4 * partialids, int* active, int* producers, int* partial, int maxsolutions, int* solutions, ushort4(*solutionids)[4], int multiprocessors);

class Piece
{
public:
    Piece(std::initializer_list<int3> coords)
    {
        this->coords = coords;
    }

    static int3 rotateA(int3 coords) { return { coords.x, coords.y, coords.z }; }
    static int3 rotateB(int3 coords) { return { coords.z, coords.x, coords.y }; }
    static int3 rotateC(int3 coords) { return { coords.y, coords.z, coords.x }; }

    static int3 rotateD(int3 coords) { return { -coords.x, -coords.y, coords.z }; }
    static int3 rotateE(int3 coords) { return { -coords.z, -coords.x, coords.y }; }
    static int3 rotateF(int3 coords) { return { -coords.y, -coords.z, coords.x }; }

    static int3 rotateG(int3 coords) { return { -coords.x, coords.y, -coords.z }; }
    static int3 rotateH(int3 coords) { return { -coords.z, coords.x, -coords.y }; }
    static int3 rotateI(int3 coords) { return { -coords.y, coords.z, -coords.x }; }

    static int3 rotateJ(int3 coords) { return { coords.x, -coords.y, -coords.z }; }
    static int3 rotateK(int3 coords) { return { coords.z, -coords.x, -coords.y }; }
    static int3 rotateL(int3 coords) { return { coords.y, -coords.z, -coords.x }; }

    static int3 rotateM(int3 coords) { return { -coords.x, coords.z, coords.y }; }
    static int3 rotateN(int3 coords) { return { -coords.y, coords.x, coords.z }; }
    static int3 rotateO(int3 coords) { return { -coords.z, coords.y, coords.x }; }

    static int3 rotateP(int3 coords) { return { coords.x, -coords.z, coords.y }; }
    static int3 rotateQ(int3 coords) { return { coords.y, -coords.x, coords.z }; }
    static int3 rotateR(int3 coords) { return { coords.z, -coords.y, coords.x }; }

    static int3 rotateS(int3 coords) { return { coords.x, coords.z, -coords.y }; }
    static int3 rotateT(int3 coords) { return { coords.y, coords.x, -coords.z }; }
    static int3 rotateU(int3 coords) { return { coords.z, coords.y, -coords.x }; }

    static int3 rotateV(int3 coords) { return { -coords.x, -coords.z, -coords.y }; }
    static int3 rotateW(int3 coords) { return { -coords.y, -coords.x, -coords.z }; }
    static int3 rotateX(int3 coords) { return { -coords.z, -coords.y, -coords.x }; }

    std::set<uint64_t> CreateOccupiedSet(int xshifts, int yshifts, int zshifts, int rots)
    {
        std::set<uint64_t> result;

        int3(*rotations[24])(int3) = {
            rotateA,
            rotateB,
            rotateC,
            rotateD,
            rotateE,
            rotateF,
            rotateG,
            rotateH,
            rotateI,
            rotateJ,
            rotateK,
            rotateL,
            rotateM,
            rotateN,
            rotateO,
            rotateP,
            rotateQ,
            rotateR,
            rotateS,
            rotateT,
            rotateU,
            rotateV,
            rotateW,
            rotateX
        };

        for (int xshift = 0; xshift < xshifts; xshift++)
        {
            for (int yshift = 0; yshift < yshifts; yshift++)
            {
                for (int zshift = 0; zshift < zshifts; zshift++)
                {
                    for (int rot = 0; rot < rots; rot++)
                    {
                        uint64_t occupied = 0;
                        for (auto& coord : this->coords)
                        {
                            int3 shifted = { coord.x + (xshift << 1), coord.y + (yshift << 1), coord.z + (zshift << 1) };
                            int3 rotated = rotations[rot](shifted);
                            int x = (rotated.x + 3) >> 1;
                            int y = (rotated.y + 3) >> 1;
                            int z = (rotated.z + 3) >> 1;
                            uint64_t mask = 1ULL << ((((z << 2) + y) << 2) + x);
                            occupied |= mask;
                        }

                        result.insert(occupied);
                    }
                }
            }
        }

        return result;
    }

    std::vector<int3> coords;
};

void PrintSolution(const ulonglong1* occupiedfromid, int level, const int* ids)
{
    int piece[13];
    uint64_t occupied[13] = {};

    for (int i = 0; i < level; i++)
    {
        piece[i] = (ids[i] >> 5) & 15;
        occupied[i] = occupiedfromid[ids[i]].x;
        printf("%i ", ids[i]);
    }

    printf("\n");

    char solution[4][4][4];
    for (int x = 0; x < 4; x++)
    {
        for (int y = 0; y < 4; y++)
        {
            for (int z = 0; z < 4; z++)
            {
                solution[z][y][x] = '-';
                uint64_t mask = 1ULL << ((((z << 2) + y) << 2) + x);
                for (int i = 0; i < level; i++)
                {
                    if (occupied[i] & mask)
                    {
                        solution[z][y][x] = 'A' + piece[i];
                    }
                }
            }
        }
    }

    for (int y = 3; y >= 0; y--)
    {
        printf("%c%c%c%c %c%c%c%c %c%c%c%c %c%c%c%c\n", solution[0][y][0], solution[0][y][1], solution[0][y][2], solution[0][y][3], solution[1][y][0], solution[1][y][1], solution[1][y][2], solution[1][y][3], solution[2][y][0], solution[2][y][1], solution[2][y][2], solution[2][y][3], solution[3][y][0], solution[3][y][1], solution[3][y][2], solution[3][y][3]);
    }

    printf("\n");
}

int main()
{
    std::set<uint64_t> occupiedset_bedlam[13] =
    {
        Piece({ { -3, -3, -3 }, { -3, -1, -3 }, { -3, 1, -3 }, { -3, 1, -1 }, { -1, 1, -3 } }).CreateOccupiedSet(3, 2, 3, 1),
        Piece({ { -3, -3, -3 }, { -1, -3, -3 }, { -1, -1, -3 }, { 1, -1, -3 }, { 1, -1, -1 } }).CreateOccupiedSet(2, 3, 3, 24),
        Piece({ { -3, -3, -3 }, { -3, -1, -3 }, { -1, -1, -3 }, { 1, -1, -3 }, { 1, -1, -1 } }).CreateOccupiedSet(2, 3, 3, 24),
        Piece({ { -3, -3, -3 }, { -1, -3, -3 }, { 1, -3, -3 }, { -1, -1, -3 }, { -1, -1, -1 } }).CreateOccupiedSet(2, 3, 3, 24),
        Piece({ { -3, -3, -3 }, { -3, -1, -3 }, { -3, 1, -3 }, { -1, 1, -3 }, { -1, 1, -1 } }).CreateOccupiedSet(3, 2, 3, 24),
        Piece({ { -3, -1, -3 }, { -1, -1, -3 }, { -1, -3, -3 }, { 1, -1, -3 }, { 1, 1, -3 } }).CreateOccupiedSet(2, 2, 4, 24),
        Piece({ { -3, -3, -3 }, { -1, -3, -3 }, { -1, -1, -3 }, { -1, -1, -1 }, { 1, -1, -1 } }).CreateOccupiedSet(2, 3, 3, 24),
        Piece({ { -3, -1, -3 }, { -1, -1, -3 }, { -1, -3, -3 }, { 1, -1, -3 }, { 1, -1, -1 } }).CreateOccupiedSet(2, 3, 3, 24),
        Piece({ { -3, -1, -3 }, { -1, -1, -3 }, { -1, -3, -3 }, { -1, -1, -1 }, { 1, -1, -3 } }).CreateOccupiedSet(2, 3, 3, 24),
        Piece({ { -3, -3, -3 }, { -1, -3, -3 }, { -1, -1, -3 }, { -1, -1, -1 }, { 1, -1, -3 } }).CreateOccupiedSet(2, 3, 3, 24),
        Piece({ { -3, -3, -3 }, { -1, -3, -3 }, { -1, -1, -3 }, { 1, -1, -3 }, { 1, 1, -3 } }).CreateOccupiedSet(2, 2, 4, 24),
        Piece({ { -3, -1, -3 }, { -1, -1, -3 }, { -1, -3, -3 }, { 1, -1, -3 }, { -1, 1, -3 } }).CreateOccupiedSet(2, 2, 4, 24),
        Piece({ { -3, -1, -3 }, { -1, -1, -3 }, { -1, -3, -3 }, { -3, -1, -1 } }).CreateOccupiedSet(3, 3, 3, 24)
    };

    std::set<uint64_t> occupiedset_abraxis[13] =
    {
        Piece({ { -3, -3, -3 }, { -3, -1, -3 }, { -3, 1, -3 }, { -1, 1, -3 }, { -3, 1, -1 } }).CreateOccupiedSet(3, 2, 3, 1),
        Piece({ { -1, -3, -3 }, { -1, -1, -3 }, { -1, 1, -3 }, { -3, -1, -3 }, { -3, -1, -1 } }).CreateOccupiedSet(3, 2, 3, 24),
        Piece({ { -3, -3, -3 }, { -3, -1, -3 }, { -3, 1, -3 }, { -1, -1, -3 }, { -3, -1, -1 } }).CreateOccupiedSet(3, 2, 3, 24),
        Piece({ { -1, -3, -3 }, { -1, -1, -3 }, { -3, -1, -3 }, { -3, 1, -3 }, { -3, -1, -1 } }).CreateOccupiedSet(3, 2, 3, 24),
        Piece({ { -3, -3, -3 }, { -3, -1, -3 }, { -1, -1, -3 }, { -1, 1, -3 }, { -3, -1, -1 } }).CreateOccupiedSet(3, 2, 3, 24),
        Piece({ { -3, -3, -3 }, { -3, -1, -3 }, { -3, 1, -3 }, { -1, -3, -3 }, { -3, -1, -1 } }).CreateOccupiedSet(3, 2, 3, 24),
        Piece({ { -3, -3, -3 }, { -3, -1, -3 }, { -3, 1, -3 }, { -1, 1, -3 }, { -3, -1, -1 } }).CreateOccupiedSet(3, 2, 3, 24),
        Piece({ { -3, -1, -3 }, { -1, -1, -3 }, { -1, 1, -3 }, { -3, -3, -1 }, { -3, -1, -1 } }).CreateOccupiedSet(3, 2, 3, 24),
        Piece({ { -1, -3, -3 }, { -1, -1, -3 }, { -3, -1, -3 }, { -3, -1, -1 }, { -3, 1, -1 } }).CreateOccupiedSet(3, 2, 3, 24),
        Piece({ { -3, -3, -3 }, { -1, -3, -3 }, { -1, -1, -3 }, { -1, 1, -3 }, { -3, -3, -1 } }).CreateOccupiedSet(3, 2, 3, 24),
        Piece({ { -3, -3, -3 }, { -3, -1, -3 }, { -3, 1, -3 }, { -1, -3, -3 }, { -3, 1, -1 } }).CreateOccupiedSet(3, 2, 3, 24),
        Piece({ { -1, -3, -3 }, { -1, -1, -3 }, { -3, -1, -3 }, { -3, 1, -3 }, { -3, 1, -1 } }).CreateOccupiedSet(3, 2, 3, 24),
        Piece({ { -3, -3, -3 }, { -3, -1, -3 }, { -3, -1, -1 }, { -1, -1, -3 } }).CreateOccupiedSet(3, 3, 3, 24)
    };

    auto& occupiedset = occupiedset_bedlam;

    ulonglong1 occupiedfromid[64 * 512] = {};
    uint64_t unsetmask = 0;
    uint64_t setmask = 1;
    for (int target = 0; target < 64; target++)
    {
        for (int piece = 0; piece < 13; piece++)
        {
            int id = (target << 9) + (piece << 5);
            for (auto& occupied : occupiedset[piece])
            {
                if (!(occupied & unsetmask) && (occupied & setmask))
                {
                    occupiedfromid[id].x = occupied;
                    id++;
                }
            }
        }
        unsetmask |= setmask;
        setmask = setmask << 1;
    }

    int device = 0;
    cudaGetDevice(&device);

    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, device);

    printf("bedlam v2.2\n");
    printf("Copyright (c) 2026 Sam Hawker <smhwkr@googlemail.com>. All rights reserved.\n\n");
    printf("%s\n", prop.name);
    printf("MultiProcessorCount: %i\n", prop.multiProcessorCount);
    printf("MaxThreadsPerBlock: %i\n", prop.maxThreadsPerBlock);
    printf("MaxThreadsPerMultiProcessor: %i\n", prop.maxThreadsPerMultiProcessor);
    printf("MaxSharedMemPerMultiprocessor: %lli\n\n", prop.sharedMemPerMultiprocessor);

    int multiprocessors = prop.multiProcessorCount;

    ulonglong1* dev_occupiedfromid = nullptr;
    cudaMalloc(&dev_occupiedfromid, 64 * 512 * sizeof(ulonglong1));

    cudaMemcpy(dev_occupiedfromid, occupiedfromid, 64 * 512 * sizeof(ulonglong1), cudaMemcpyHostToDevice);

    const int maxpartials = 1 << 22;
    const int maxsolutions = 1 << 17;

    int* dev_partials = nullptr;
    cudaMalloc(&dev_partials, sizeof(int));

    ushort1* dev_partialpieces = nullptr;
    cudaMalloc(&dev_partialpieces, maxpartials * sizeof(ushort1));

    ulonglong1* dev_partialoccupied = nullptr;
    cudaMalloc(&dev_partialoccupied, maxpartials * sizeof(ulonglong1));

    ushort4* dev_partialids = nullptr;
    cudaMalloc(&dev_partialids, maxpartials * sizeof(ushort4));

    int* host_active = nullptr;
    cudaMallocHost(&host_active, sizeof(int));

    int* host_producers = nullptr;
    cudaMallocHost(&host_producers, sizeof(int));

    int* dev_partial = nullptr;
    cudaMalloc(&dev_partial, sizeof(int));

    int* dev_solutions = nullptr;
    cudaMalloc(&dev_solutions, sizeof(int));

    ushort4(*dev_solutionids)[4] = nullptr;
    cudaMalloc(&dev_solutionids, maxsolutions * sizeof(ushort4[4]));

    LARGE_INTEGER frequency;
    LARGE_INTEGER start, end;

    timeBeginPeriod(1);

    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);

    cudaMemsetAsync(dev_partials, 0, sizeof(int));

    cudaMemsetAsync(dev_partial, 0, sizeof(int));
    cudaMemsetAsync(dev_solutions, 0, sizeof(int));

    cudaPartialSolve(dev_occupiedfromid, maxpartials, dev_partials, dev_partialpieces, dev_partialoccupied, dev_partialids);

    cudaSolve(dev_occupiedfromid, dev_partials, dev_partialpieces, dev_partialoccupied, dev_partialids, host_active, host_producers, dev_partial, maxsolutions, dev_solutions, dev_solutionids, multiprocessors);

    printf("ms\tactive\tproducers\n");

    while (cudaStreamQuery(0) == cudaErrorNotReady)
    {
        QueryPerformanceCounter(&end);
        double elapsed_ms = (end.QuadPart - start.QuadPart) * 1000.0 / frequency.QuadPart;
        printf("%0.1lf\t%i\t%i\n", elapsed_ms, *host_active, *host_producers);
        Sleep(5);
    }

    QueryPerformanceCounter(&end);
    double elapsed_ms = (end.QuadPart - start.QuadPart) * 1000.0 / frequency.QuadPart;
    printf("%0.1lf\t%i\t%i\n\n", elapsed_ms, *host_active, *host_producers);

    int partials = 0;
    cudaMemcpy(&partials, dev_partials, sizeof(int), cudaMemcpyDeviceToHost);

    int solutions = 0;
    cudaMemcpy(&solutions, dev_solutions, sizeof(int), cudaMemcpyDeviceToHost);

    auto solutionids = std::make_unique<uint16_t[][16]>(maxsolutions);
    cudaMemcpy(solutionids.get(), dev_solutionids, min(solutions, maxsolutions) * sizeof(uint16_t[16]), cudaMemcpyDeviceToHost);

    printf("%i partial solutions found\n", partials);

    printf("%i solutions found\n", solutions);

    for (int solution = 0; solution < min(solutions, maxsolutions); solution++)
    {
        int ids[13];
        for (int level = 0; level < 13; level++)
        {
            ids[level] = static_cast<int>(solutionids[solution][level]);
        }
        //printf("#%i\n", solution + 1);
        //PrintSolution(occupiedfromid, 13, ids);
    }

    solutionids = nullptr;

    cudaFree(dev_solutionids);

    cudaFree(dev_solutions);

    cudaFreeHost(host_producers);

    cudaFreeHost(host_active);

    cudaFree(dev_partial);

    cudaFree(dev_partialids);

    cudaFree(dev_partialoccupied);

    cudaFree(dev_partialpieces);

    cudaFree(dev_partials);

    cudaFree(dev_occupiedfromid);

    return 0;
}
