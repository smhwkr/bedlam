// bedlam.cpp
// Copyright (c) 2026 Sam Hawker <smhwkr@googlemail.com>. All rights reserved.

#include <windows.h>

#include <stdio.h>

#include <cuda.h>
#include <cuda_runtime.h>

#include <memory>
#include <vector>
#include <set>
#include <array>
#include <bit>

extern "C" void cudaPartialSolve(const ulonglong1 * occupiedfromcid, int maxpartials, int* partials, ushort2 * partialpieces, ulonglong1 * partialoccupied, ushort4 * partialcids);

extern "C" void cudaSolve(const ulonglong1 * occupiedfromrid, const uchar1 * rotfromcid, const int* partials, const ushort2 * partialpieces, const ulonglong1 * partialoccupied, const ushort4 * partialcids, int* active, int* producers, int* partial, int maxsolutions, int* solutions, ushort4(*solutioncids)[4], int multiprocessors);

class Placement
{
public:
    Placement(uint64_t occupied, uint8_t rot)
    {
        this->occupied = occupied;
        this->rot = rot + 1;
    }

    bool operator<(const Placement& other) const
    {
        return occupied < other.occupied;
    }

    uint64_t occupied;
    uint8_t rot;
};

class Piece
{
public:
    Piece(std::initializer_list<int3> coords, int rotate)
    {
        this->coords = coords;
        this->rotate = rotate;

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

        for (int rot = 0; rot < 24; rot++)
        {
            std::vector<int3> rcoords(this->coords);

            int minx = 3;
            int maxx = 0;
            int miny = 3;
            int maxy = 0;
            int minz = 3;
            int maxz = 0;

            // rotate and compute bounds
            for (auto& rcoord : rcoords)
            {
                rcoord = rotations[rot](rcoord);

                minx = min(minx, rcoord.x);
                maxx = max(maxx, rcoord.x);
                miny = min(miny, rcoord.y);
                maxy = max(maxy, rcoord.y);
                minz = min(minz, rcoord.z);
                maxz = max(maxz, rcoord.z);
            }

            occupied[rot] = 0ULL;

            // shift into (0, 0, 0) corner
            for (auto& rcoord : rcoords)
            {
                rcoord = int3({ rcoord.x - minx, rcoord.y - miny, rcoord.z - minz });
                uint64_t mask = 1ULL << ((((rcoord.z << 2) + rcoord.y) << 2) + rcoord.x);
                occupied[rot] |= mask;
            }

            // store all the unique placements
            if (!rot || rotate)
            {
                int xshifts = 4 - maxx + minx;
                int yshifts = 4 - maxy + miny;
                int zshifts = 4 - maxz + minz;

                for (int xshift = 0; xshift < xshifts; xshift++)
                {
                    for (int yshift = 0; yshift < yshifts; yshift++)
                    {
                        for (int zshift = 0; zshift < zshifts; zshift++)
                        {
                            int shift = (((zshift << 2) + yshift) << 2) + xshift;
                            placements.insert(Placement(occupied[rot] << shift, static_cast<uint8_t>(rot)));
                        }
                    }
                }
            }
        }
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

    std::vector<int3> coords;
    bool rotate;
    std::array<uint64_t, 24> occupied;
    std::set<Placement> placements;
};

void PrintSolution(const ulonglong1* occupiedfromcid, int level, const int* cids)
{
    int piece[13];
    uint64_t occupied[13] = {};

    for (int i = 0; i < level; i++)
    {
        piece[i] = (cids[i] / 25) % 13;
        occupied[i] = occupiedfromcid[cids[i]].x;
        printf("%i ", cids[i]);
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
    Piece pieces_bedlam[13] =
    {
        Piece({ { 0, 0, 0 }, { 0, 1, 0 }, { 0, 2, 0 }, { 0, 2, 1 }, { 1, 2, 0 } }, false),
        Piece({ { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 2, 1, 0 }, { 2, 1, 1 } }, true),
        Piece({ { 0, 0, 0 }, { 0, 1, 0 }, { 1, 1, 0 }, { 2, 1, 0 }, { 2, 1, 1 } }, true),
        Piece({ { 0, 0, 0 }, { 1, 0, 0 }, { 2, 0, 0 }, { 1, 1, 0 }, { 1, 1, 1 } }, true),
        Piece({ { 0, 0, 0 }, { 0, 1, 0 }, { 0, 2, 0 }, { 1, 2, 0 }, { 1, 2, 1 } }, true),
        Piece({ { 0, 1, 0 }, { 1, 1, 0 }, { 1, 0, 0 }, { 2, 1, 0 }, { 2, 2, 0 } }, true),
        Piece({ { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 1, 1, 1 }, { 2, 1, 1 } }, true),
        Piece({ { 0, 1, 0 }, { 1, 1, 0 }, { 1, 0, 0 }, { 2, 1, 0 }, { 2, 1, 1 } }, true),
        Piece({ { 0, 1, 0 }, { 1, 1, 0 }, { 1, 0, 0 }, { 1, 1, 1 }, { 2, 1, 0 } }, true),
        Piece({ { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 1, 1, 1 }, { 2, 1, 0 } }, true),
        Piece({ { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 2, 1, 0 }, { 2, 2, 0 } }, true),
        Piece({ { 0, 1, 0 }, { 1, 1, 0 }, { 1, 0, 0 }, { 2, 1, 0 }, { 1, 2, 0 } }, true),
        Piece({ { 0, 1, 0 }, { 1, 1, 0 }, { 1, 0, 0 }, { 0, 1, 1 } }, true)
    };

    Piece pieces_abraxis[13] =
    {
        Piece({ { 0, 0, 0 }, { 0, 1, 0 }, { 0, 2, 0 }, { 1, 2, 0 }, { 0, 2, 1 } }, false),
        Piece({ { 1, 0, 0 }, { 1, 1, 0 }, { 1, 2, 0 }, { 0, 1, 0 }, { 0, 1, 1 } }, true),
        Piece({ { 0, 0, 0 }, { 0, 1, 0 }, { 0, 2, 0 }, { 1, 1, 0 }, { 0, 1, 1 } }, true),
        Piece({ { 1, 0, 0 }, { 1, 1, 0 }, { 0, 1, 0 }, { 0, 2, 0 }, { 0, 1, 1 } }, true),
        Piece({ { 0, 0, 0 }, { 0, 1, 0 }, { 1, 1, 0 }, { 1, 2, 0 }, { 0, 1, 1 } }, true),
        Piece({ { 0, 0, 0 }, { 0, 1, 0 }, { 0, 2, 0 }, { 1, 0, 0 }, { 0, 1, 1 } }, true),
        Piece({ { 0, 0, 0 }, { 0, 1, 0 }, { 0, 2, 0 }, { 1, 2, 0 }, { 0, 1, 1 } }, true),
        Piece({ { 0, 1, 0 }, { 1, 1, 0 }, { 1, 2, 0 }, { 0, 0, 1 }, { 0, 1, 1 } }, true),
        Piece({ { 1, 0, 0 }, { 1, 1, 0 }, { 0, 1, 0 }, { 0, 1, 1 }, { 0, 2, 1 } }, true),
        Piece({ { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 1, 2, 0 }, { 0, 0, 1 } }, true),
        Piece({ { 0, 0, 0 }, { 0, 1, 0 }, { 0, 2, 0 }, { 1, 0, 0 }, { 0, 2, 1 } }, true),
        Piece({ { 1, 0, 0 }, { 1, 1, 0 }, { 0, 1, 0 }, { 0, 2, 0 }, { 0, 2, 1 } }, true),
        Piece({ { 0, 0, 0 }, { 0, 1, 0 }, { 0, 1, 1 }, { 1, 1, 0 } }, true)
    };

    auto& pieces = pieces_bedlam;

    ulonglong1 occupiedfromrid[325] = {};
    for (int piece = 0; piece < 13; piece++)
    {
        for (int rot = 0; rot < 24; rot++)
        {
            uint64_t occupied = pieces[piece].occupied[rot];
            occupiedfromrid[piece * 25 + rot + 1].x = occupied >> std::countr_zero(occupied);
        }
    }

    uchar1 rotfromcid[64 * 325] = {};
    ulonglong1 occupiedfromcid[64 * 325] = {};
    uint64_t unsetmask = 0;
    uint64_t setmask = 1;
    for (int target = 0; target < 64; target++)
    {
        for (int piece = 0; piece < 13; piece++)
        {
            int cid = target * 325 + piece * 25;
            for (auto& placement : pieces[piece].placements)
            {
                uint64_t occupied = placement.occupied;
                if (!(occupied & unsetmask) && (occupied & setmask))
                {
                    rotfromcid[cid].x = placement.rot;
                    occupiedfromcid[cid].x = occupied;
                    cid++;
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

    printf("bedlam v2.7\n");
    printf("Copyright (c) 2026 Sam Hawker <smhwkr@googlemail.com>. All rights reserved.\n\n");
    printf("%s\n", prop.name);
    printf("MultiProcessorCount: %i\n", prop.multiProcessorCount);
    printf("MaxThreadsPerBlock: %i\n", prop.maxThreadsPerBlock);
    printf("MaxThreadsPerMultiProcessor: %i\n", prop.maxThreadsPerMultiProcessor);
    printf("MaxSharedMemPerMultiprocessor: %lli\n\n", prop.sharedMemPerMultiprocessor);

    int multiprocessors = prop.multiProcessorCount;

    ulonglong1* dev_occupiedfromrid = nullptr;
    cudaMalloc(&dev_occupiedfromrid, 325 * sizeof(ulonglong1));

    cudaMemcpy(dev_occupiedfromrid, occupiedfromrid, 325 * sizeof(ulonglong1), cudaMemcpyHostToDevice);

    uchar1* dev_rotfromcid = nullptr;
    cudaMalloc(&dev_rotfromcid, 64 * 512 * sizeof(uchar1));

    cudaMemcpy(dev_rotfromcid, rotfromcid, 64 * 325 * sizeof(uchar1), cudaMemcpyHostToDevice);

    ulonglong1* dev_occupiedfromcid = nullptr;
    cudaMalloc(&dev_occupiedfromcid, 64 * 325 * sizeof(ulonglong1));

    cudaMemcpy(dev_occupiedfromcid, occupiedfromcid, 64 * 325 * sizeof(ulonglong1), cudaMemcpyHostToDevice);

    const int maxpartials = 1 << 25;
    const int maxsolutions = 1 << 17;

    int* dev_partials = nullptr;
    cudaMalloc(&dev_partials, sizeof(int));

    ushort2* dev_partialpieces = nullptr;
    cudaMalloc(&dev_partialpieces, maxpartials * sizeof(ushort2));

    ulonglong1* dev_partialoccupied = nullptr;
    cudaMalloc(&dev_partialoccupied, maxpartials * sizeof(ulonglong1));

    ushort4* dev_partialcids = nullptr;
    cudaMalloc(&dev_partialcids, maxpartials * sizeof(ushort4));

    int* host_active = nullptr;
    cudaMallocHost(&host_active, sizeof(int));

    int* host_producers = nullptr;
    cudaMallocHost(&host_producers, sizeof(int));

    int* dev_partial = nullptr;
    cudaMalloc(&dev_partial, sizeof(int));

    int* dev_solutions = nullptr;
    cudaMalloc(&dev_solutions, sizeof(int));

    ushort4(*dev_solutioncids)[4] = nullptr;
    cudaMalloc(&dev_solutioncids, maxsolutions * sizeof(ushort4[4]));

    LARGE_INTEGER frequency;
    LARGE_INTEGER start, end;

    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);

    timeBeginPeriod(1);

    cudaMemsetAsync(dev_partials, 0, sizeof(int));

    cudaMemsetAsync(dev_partial, 0, sizeof(int));
    cudaMemsetAsync(dev_solutions, 0, sizeof(int));

    cudaPartialSolve(dev_occupiedfromcid, maxpartials, dev_partials, dev_partialpieces, dev_partialoccupied, dev_partialcids);

    cudaSolve(dev_occupiedfromrid, dev_rotfromcid, dev_partials, dev_partialpieces, dev_partialoccupied, dev_partialcids, host_active, host_producers, dev_partial, maxsolutions, dev_solutions, dev_solutioncids, multiprocessors);

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

    auto solutioncids = std::make_unique<uint16_t[][16]>(maxsolutions);
    cudaMemcpy(solutioncids.get(), dev_solutioncids, min(solutions, maxsolutions) * sizeof(uint16_t[16]), cudaMemcpyDeviceToHost);

    printf("%i partial solutions found\n", partials);

    printf("%i solutions found\n", solutions);

    for (int solution = 0; solution < min(solutions, maxsolutions); solution++)
    {
        int cids[13];
        for (int level = 0; level < 13; level++)
        {
            cids[level] = static_cast<int>(solutioncids[solution][level]);
        }
        //printf("#%i\n", solution + 1);
        //PrintSolution(occupiedfromcid, 13, cids);
    }

    solutioncids = nullptr;

    cudaFree(dev_solutioncids);

    cudaFree(dev_solutions);

    cudaFreeHost(host_producers);

    cudaFreeHost(host_active);

    cudaFree(dev_partial);

    cudaFree(dev_partialcids);

    cudaFree(dev_partialoccupied);

    cudaFree(dev_partialpieces);

    cudaFree(dev_partials);

    cudaFree(dev_occupiedfromcid);

    cudaFree(dev_rotfromcid);

    cudaFree(dev_occupiedfromrid);

    return 0;
}
