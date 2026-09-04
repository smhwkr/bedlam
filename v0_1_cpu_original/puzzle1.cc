// puzzle1.cc - creates bitmasks for all possible positions of all pieces.
//            - saves them to disk.
//            - then reloads them.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

typedef unsigned long long mask64;

void mask64fprint(FILE *fp, mask64 printmask) {
  for(int i=0;i<64;i++) {
    if((printmask & ((mask64)1<<i)))
      fprintf(fp, "1");
    else
      fprintf(fp, "0");
  }
  fprintf(fp, "\n");
}

void mask64matrixfprint(FILE *fp, mask64 printmask) {
  for(int x=0;x<4;x++) {
    for(int y=0;y<4;y++) {
      for(int z=0;z<4;z++) {
        if((printmask & ((mask64)1<<(x+4*y+16*z)))!=(mask64)0) {
          fprintf(fp, "(%i, %i, %i)\n", x, y, z);
	}
      }
    }
  }
  fprintf(fp, "\n");
}

void mask64gridfprint(FILE *fp, mask64 printmask) {
  for(int y=0;y<4;y++) {
    for(int z=0;z<4;z++) {
      for(int x=0;x<4;x++) {
        if((printmask & ((mask64)1<<(x+4*y+16*z)))!=(mask64)0)
          fprintf(fp, "X");
        else
          fprintf(fp, ".");
      }
      fprintf(fp, "    ");
    }
    fprintf(fp, "\n");
  }
  fprintf(fp, "\n");
}

class matrix {
public:
  matrix() {
    rows=0;
    cols=0;
    data=NULL;
    allocated=false;
  }
  matrix(int _rows, int _cols) {
    rows=_rows;
    cols=_cols;
    data=(int *)malloc(sizeof(int)*rows*cols);
    allocated=true;
  }
  matrix(int _rows, int _cols, int *_data) {
    rows=_rows;
    cols=_cols;
    data=_data;
    allocated=false;
  }
  matrix(const matrix &_matrix) {
    rows=_matrix.rows;
    cols=_matrix.cols;
    data=(int *)malloc(sizeof(int)*rows*cols);
    memcpy(data, _matrix.data, sizeof(int)*rows*cols);
    allocated=true;
  }
  ~matrix() {
    if(allocated)
      free(data);
  }
  int at(int _row, int _col) const {
    return data[rows*_col+_row];
  }
  void set(int _row, int _col, int _val) {
    data[rows*_col+_row]=_val;
  }
  matrix operator *(const matrix &_matrix) const {
    matrix _newmatrix(rows, _matrix.cols);
    for(int _col=0;_col<_newmatrix.cols;_col++) {
      for(int _row=0;_row<_newmatrix.rows;_row++) {
        int _val=0;
        for(int _pos=0;_pos<_matrix.rows;_pos++)
          _val+=at(_row, _pos)*_matrix.at(_pos, _col);
        _newmatrix.set(_row, _col, _val);
      }
    }
    return _newmatrix;
  }
  mask64 mask() const {
    mask64 _mask=0;
    for(int _col=0;_col<cols;_col++) {
      int x=at(0, _col);
      int y=at(1, _col);
      int z=at(2, _col);
      mask64 cubemask=((mask64)1<<(x+4*y+16*z));
      _mask|=cubemask;
    }
    return _mask;
  }
  void print() const {
    for(int _row=0;_row<rows;_row++) {
      for(int _col=0;_col<cols;_col++) {
        printf(" %2i", at(_row, _col));
      }
      printf("\n");
    }
  }
  int rows;
  int cols;
  int *data;
  bool allocated;
};

matrix *mpieces[13];
matrix *mrotations[24];

int dpieces[]={ 3, 5, 0, 0, 0, 0, 1, 0, 0, 2, 0, 0, 2, 1, 1, 2, 0,
                3, 5, 0, 0, 0, 1, 0, 0, 1, 1, 0, 2, 1, 0, 2, 1, 1,
                3, 5, 0, 0, 0, 0, 1, 0, 1, 1, 0, 2, 1, 0, 2, 1, 1,
                3, 5, 0, 0, 0, 1, 0, 0, 2, 0, 0, 1, 1, 0, 1, 1, 1,
                3, 5, 0, 0, 0, 0, 1, 0, 0, 2, 0, 1, 2, 0, 1, 2, 1,
                3, 5, 0, 1, 0, 1, 1, 0, 1, 0, 0, 2, 1, 0, 2, 2, 0,
                3, 5, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 2, 1, 1,
                3, 5, 0, 1, 0, 1, 1, 0, 1, 0, 0, 2, 1, 0, 2, 1, 1,
                3, 5, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 2, 1, 0,
                3, 5, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 2, 1, 0,
                3, 5, 0, 0, 0, 1, 0, 0, 1, 1, 0, 2, 1, 0, 2, 2, 0,
                3, 5, 0, 1, 0, 1, 1, 0, 1, 0, 0, 2, 1, 0, 1, 2, 0,
                3, 4, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1 };

int drotations[]={ 3, 3,  1,  0,  0,  0,  1,  0,  0,  0,  1,
                   3, 3,  1,  0,  0,  0,  0, -1,  0,  1,  0,
                   3, 3,  1,  0,  0,  0, -1,  0,  0,  0, -1,
                   3, 3,  1,  0,  0,  0,  0,  1,  0, -1,  0,
                   3, 3, -1,  0,  0,  0, -1,  0,  0,  0,  1,
                   3, 3, -1,  0,  0,  0,  0,  1,  0,  1,  0,
                   3, 3, -1,  0,  0,  0,  1,  0,  0,  0, -1,
                   3, 3, -1,  0,  0,  0,  0, -1,  0, -1,  0,
                   3, 3,  0,  0,  1,  0,  1,  0, -1,  0,  0,
                   3, 3,  0,  0,  1,  1,  0,  0,  0,  1,  0,
                   3, 3,  0,  0,  1,  0, -1,  0,  1,  0,  0,
                   3, 3,  0,  0,  1, -1,  0,  0,  0, -1,  0,
                   3, 3,  0,  0, -1,  0, -1,  0, -1,  0,  0,
                   3, 3,  0,  0, -1, -1,  0,  0,  0,  1,  0,
                   3, 3,  0,  0, -1,  0,  1,  0,  1,  0,  0,
                   3, 3,  0,  0, -1,  1,  0,  0,  0, -1,  0,
                   3, 3,  0, -1,  0,  0,  0,  1, -1,  0,  0,
                   3, 3,  0, -1,  0,  1,  0,  0,  0,  0,  1,
                   3, 3,  0, -1,  0,  0,  0, -1,  1,  0,  0,
                   3, 3,  0, -1,  0, -1,  0,  0,  0,  0, -1,
                   3, 3,  0,  1,  0,  0,  0,  1,  1,  0,  0,
                   3, 3,  0,  1,  0,  1,  0,  0,  0,  0, -1,
                   3, 3,  0,  1,  0,  0,  0, -1, -1,  0,  0,
                   3, 3,  0,  1,  0, -1,  0,  0,  0,  0,  1};

class piece {
public:
  int nummasks;
  mask64 *masks;
};

piece pieces[13];

int masksused[13];

int solutions=0;

void creatematrices() {
  int i;
  i=0;
  for(int p=0;p<13;p++) {
    int rows=dpieces[i];
    i++;
    int cols=dpieces[i];
    i++;
    mpieces[p]=new matrix(rows, cols, &dpieces[i]);
    i+=rows*cols;
  }
  i=0;
  for(int r=0;r<24;r++) {
    int rows=drotations[i];
    i++;
    int cols=drotations[i];
    i++;
    mrotations[r]=new matrix(rows, cols, &drotations[i]);
    i+=rows*cols;
  }
}

void freematrices() {
  for(int p=0;p<13;p++)
    delete mpieces[p];
  for(int r=0;r<24;r++)
    delete mrotations[r];
}

void savemasks() {
  for(int p=0;p<13;p++) {
    char filename[256];
    sprintf(filename, "piece%02i.dat", p);
    printf("Writing file: %s\n", filename);
    FILE *fp=fopen(filename, "wb");
    int nummasks=0;
    fwrite(&nummasks, sizeof(int), 1, fp);
    for(int r=0;r<24;r++) {
      matrix mrotated(*mrotations[r]**mpieces[p]);
      matrix mmoved(mrotated.rows, mrotated.cols);
      int minx=mrotated.at(0, 0);
      int miny=mrotated.at(1, 0);
      int minz=mrotated.at(2, 0);
      int maxx=minx;
      int maxy=miny;
      int maxz=minz;
      for(int pos=0;pos<mrotated.cols;pos++) {
        int x=mrotated.at(0, pos);
        int y=mrotated.at(1, pos);
        int z=mrotated.at(2, pos);
        if(x<minx)
          minx=x;
        if(y<miny)
          miny=y;
        if(z<minz)
          minz=z;
        if(x>maxx)
          maxx=x;
        if(y>maxy)
          maxy=y;
        if(z>maxz)
          maxz=z;
      }
      for(int xoff=-minx;xoff<4-maxx;xoff++) {
        for(int col=0;col<mrotated.cols;col++)
          mmoved.set(0, col, mrotated.at(0, col)+xoff);
        for(int yoff=-miny;yoff<4-maxy;yoff++) {
          for(int col=0;col<mrotated.cols;col++)
            mmoved.set(1, col, mrotated.at(1, col)+yoff);
          for(int zoff=-minz;zoff<4-maxz;zoff++) {
            for(int col=0;col<mrotated.cols;col++)
              mmoved.set(2, col, mrotated.at(2, col)+zoff);
            mask64 mask=mmoved.mask();
            fwrite(&mask, sizeof(mask64), 1, fp);
            nummasks++;
          }
	}
      }
    }
    rewind(fp);
    fwrite(&nummasks, sizeof(int), 1, fp);
    fclose(fp);
    printf("Written: %i masks\n", nummasks);
  }
}

int maskcompare(mask64 *mask1, mask64 *mask2) {
  if(*mask1==*mask2)
    return 0;
  if(*mask1<*mask2)
    return -1;
  if(*mask1>*mask2)
    return 1;
}

void loadmasks() {
  for(int p=0;p<13;p++) {
    char filename[256];
    sprintf(filename, "piece%02i.dat", p);
    FILE *fp=fopen(filename, "rb");
    fread(&pieces[p].nummasks, sizeof(int), 1, fp);
    pieces[p].masks=(mask64 *)malloc(sizeof(mask64)*pieces[p].nummasks);
    fread(pieces[p].masks, sizeof(mask64), pieces[p].nummasks, fp);
    fclose(fp);
  }
}

void freemasks() {
  for(int p=0;p<13;p++)
    free(pieces[p].masks);
}

void sortmasks() {
  for(int p=0;p<13;p++) {
    char filename[256];
    sprintf(filename, "piece%02i.dat", p);
    printf("Sorting file: %s\n", filename);
    FILE *fp;
    fp=fopen(filename, "rb");
    fread(&pieces[p].nummasks, sizeof(int), 1, fp);
    pieces[p].masks=(mask64 *)malloc(sizeof(mask64)*pieces[p].nummasks);
    fread(pieces[p].masks, sizeof(mask64), pieces[p].nummasks, fp);
    fclose(fp);
    qsort(pieces[p].masks, pieces[p].nummasks, sizeof(mask64), &maskcompare);
    fp=fopen(filename, "wb");
    fwrite(&pieces[p].nummasks, sizeof(int), 1, fp);
    mask64 lastmask=0;
    int j=0;
    for(int i=0;i<pieces[p].nummasks;i++) {
      if(pieces[p].masks[i]!=lastmask) {
        fwrite(&pieces[p].masks[i], sizeof(mask64), 1, fp);
        j++;
      }
      lastmask=pieces[p].masks[i];
    }
    rewind(fp);
    fwrite(&j, sizeof(int), 1, fp);
    printf("Sorted: %i masks\n", j);
    fclose(fp);
    free(pieces[p].masks);
  }
}

void placepiece(mask64 filledmask, int p) {
  for(int masknum=0;masknum<pieces[p].nummasks;masknum++) {
    if((filledmask & pieces[p].masks[masknum])==(mask64)0) {
      masksused[p]=masknum;
      if(p<5) {
        for(int i=0;i<=p;i++) {
          printf("%3i ", masksused[i]);
        }
        printf("\n");
      }
      if(p==12) {
        printf("Solution %i found!\n", solutions);
        char filename[256];
        sprintf(filename, "soln%04i.txt", solutions);
        FILE *of=fopen(filename, "wt");
        for(int i=0;i<=p;i++) {
          fprintf(of, "%3i : ", masksused[i]);
          mask64fprint(of, pieces[i].masks[masksused[i]]);
          mask64matrixfprint(of, pieces[i].masks[masksused[i]]);
          mask64gridfprint(of, pieces[i].masks[masksused[i]]);
          fprintf(of, "\n");
        }
        fprintf(of, "\n");
        fclose(of);
        solutions++;
      }
      else {
        placepiece(filledmask | pieces[p].masks[masknum], p+1);
      }
    }
  }
}

void main() {
  creatematrices();
  savemasks();
  freematrices();
  sortmasks();
  loadmasks();
  placepiece((mask64)0, 0);
  freemasks();
}

