#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>      // INFINITY など
#include <ctype.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#if defined(_WIN32) && !defined(__MINGW32__) && !defined(__MINGW64__)
  #include <direct.h>
  #define MKDIR(path) _mkdir(path)
#else
  #include <sys/stat.h>
  #include <sys/types.h>
  #if defined(__MINGW32__) || defined(__MINGW64__)
    #define MKDIR(path) mkdir(path)
  #else
    #define MKDIR(path) mkdir(path, 0755)
  #endif
#endif

static char* xstrdup(const char *s){
  size_t n = strlen(s) + 1;
  char *p = (char*)malloc(n);
  if(!p){ perror("malloc"); exit(1); }
  memcpy(p, s, n);
  return p;
}

// ------------------------------ 型定義 ------------------------------
typedef struct { int id; double x, y, z; } Node;

typedef struct { int id; int pid; int n1, n2, n3; } Tria3;
typedef struct { int id; int pid; int n1, n2, n3, n4, n5, n6; } Tria6;

typedef struct { Node  *data; size_t size, cap; } NodeVec;
typedef struct { Tria3 *data; size_t size, cap; } Tria3Vec;
typedef struct { Tria6 *data; size_t size, cap; } Tria6Vec;

typedef struct {
  int mid;
  int hasE, hasNu, hasRho;
  double E, nu, rho;
} Mat1;

typedef struct {
  int pid;
  int mid1;
  int hasT;
  double T; // mm（板厚）
} PShell;

typedef struct { PShell *data; size_t size, cap; } PShellVec;

typedef struct {
  int sid;      // SPC セットID
  int gid;      // GRID ID
  char comp[32];// コンポーネント文字列 ("123456")
  double d;     // 強制変位
} SPCEntry;

typedef struct {
  int sid;      // Load セットID
  int gid;      // GRID ID
  int cid;      // 座標系ID（0:基本系）
  double F;     // スカラー荷重（入力単位）
  double n1,n2,n3; // 方向ベクトル成分（無次元）
} ForceEntry;

typedef struct { SPCEntry   *data; size_t size, cap; } SPCVec;
typedef struct { ForceEntry *data; size_t size, cap; } ForceVec;

typedef struct {
  int eid;
  int n[3];       // internal node indices (0-based)
  double normal[3];
  int surface_id;
} SurfElem;

typedef struct { SurfElem *data; size_t size, cap; } SurfElemVec;

typedef struct {
  int n1, n2;      // internal node indices (0-based, sorted)
  int e1, e2;      // element indices in SurfElemVec
  int count;
} EdgeRec;

typedef struct { EdgeRec *data; size_t size, cap; } EdgeVec;

typedef struct {
  int n1, n2;
  int e1, e2;      // element indices (e2=-1 if boundary)
  int s1, s2;      // surface ids
  double dir[3];
  int rid;
} RidgeEdge;

typedef struct { RidgeEdge *data; size_t size, cap; } RidgeVec;

typedef struct { int *data; size_t size, cap; } IntVec;

// ------------------------------ ベクタ操作 ------------------------------
static void nodevec_init (NodeVec  *v){ v->data=NULL; v->size=0; v->cap=0; }
static void tria3vec_init(Tria3Vec *v){ v->data=NULL; v->size=0; v->cap=0; }
static void tria6vec_init(Tria6Vec *v){ v->data=NULL; v->size=0; v->cap=0; }

static void nodevec_push(NodeVec *v, Node n){
  if(v->size==v->cap){
    v->cap = (v->cap ? v->cap*2 : 128);
    v->data = (Node*)realloc(v->data, v->cap*sizeof(Node));
    if(!v->data){ perror("realloc"); exit(1); }
  }
  v->data[v->size++] = n;
}
static void tria3vec_push(Tria3Vec *v, Tria3 e){
  if(v->size==v->cap){
    v->cap = (v->cap ? v->cap*2 : 256);
    v->data = (Tria3*)realloc(v->data, v->cap*sizeof(Tria3));
    if(!v->data){ perror("realloc"); exit(1); }
  }
  v->data[v->size++] = e;
}
static void tria6vec_push(Tria6Vec *v, Tria6 e){
  if(v->size==v->cap){
    v->cap = (v->cap ? v->cap*2 : 128);
    v->data = (Tria6*)realloc(v->data, v->cap*sizeof(Tria6));
    if(!v->data){ perror("realloc"); exit(1); }
  }
  v->data[v->size++] = e;
}

static void pshellvec_init(PShellVec *v){ v->data=NULL; v->size=0; v->cap=0; }
static void pshellvec_push(PShellVec *v, PShell p){
  if(v->size==v->cap){
    v->cap = (v->cap ? v->cap*2 : 64);
    v->data = (PShell*)realloc(v->data, v->cap*sizeof(PShell));
    if(!v->data){ perror("realloc"); exit(1); }
  }
  v->data[v->size++] = p;
}

static void spcvec_init(SPCVec *v){ v->data=NULL; v->size=0; v->cap=0; }
static void spcvec_push(SPCVec *v, SPCEntry e){
  if(v->size==v->cap){
    v->cap = (v->cap ? v->cap*2 : 32);
    v->data = (SPCEntry*)realloc(v->data, v->cap*sizeof(SPCEntry));
    if(!v->data){ perror("realloc"); exit(1); }
  }
  v->data[v->size++] = e;
}

static void forcevec_init(ForceVec *v){ v->data=NULL; v->size=0; v->cap=0; }
static void forcevec_push(ForceVec *v, ForceEntry e){
  if(v->size==v->cap){
    v->cap = (v->cap ? v->cap*2 : 32);
    v->data = (ForceEntry*)realloc(v->data, v->cap*sizeof(ForceEntry));
    if(!v->data){ perror("realloc"); exit(1); }
  }
  v->data[v->size++] = e;
}

static void surfelemvec_init(SurfElemVec *v){ v->data=NULL; v->size=0; v->cap=0; }
static void surfelemvec_push(SurfElemVec *v, SurfElem e){
  if(v->size==v->cap){
    v->cap = (v->cap ? v->cap*2 : 256);
    v->data = (SurfElem*)realloc(v->data, v->cap*sizeof(SurfElem));
    if(!v->data){ perror("realloc"); exit(1); }
  }
  v->data[v->size++] = e;
}

static void edgevec_init(EdgeVec *v){ v->data=NULL; v->size=0; v->cap=0; }
static int edgevec_find(EdgeVec *v, int n1, int n2){
  for(size_t i=0;i<v->size;i++){
    if(v->data[i].n1==n1 && v->data[i].n2==n2){
      return (int)i;
    }
  }
  return -1;
}
static void edgevec_add(EdgeVec *v, int n1, int n2, int elem){
  if(n1>n2){ int tmp=n1; n1=n2; n2=tmp; }
  int idx=edgevec_find(v,n1,n2);
  if(idx>=0){
    EdgeRec *e=&v->data[idx];
    if(e->count==1){ e->e2=elem; }
    e->count++;
    return;
  }
  if(v->size==v->cap){
    v->cap = (v->cap ? v->cap*2 : 512);
    v->data = (EdgeRec*)realloc(v->data, v->cap*sizeof(EdgeRec));
    if(!v->data){ perror("realloc"); exit(1); }
  }
  EdgeRec rec;
  rec.n1=n1; rec.n2=n2; rec.e1=elem; rec.e2=-1; rec.count=1;
  v->data[v->size++] = rec;
}

static void ridgevec_init(RidgeVec *v){ v->data=NULL; v->size=0; v->cap=0; }
static void ridgevec_push(RidgeVec *v, RidgeEdge e){
  if(v->size==v->cap){
    v->cap = (v->cap ? v->cap*2 : 512);
    v->data = (RidgeEdge*)realloc(v->data, v->cap*sizeof(RidgeEdge));
    if(!v->data){ perror("realloc"); exit(1); }
  }
  v->data[v->size++] = e;
}

static void intvec_init(IntVec *v){ v->data=NULL; v->size=0; v->cap=0; }
static void intvec_push(IntVec *v, int x){
  if(v->size==v->cap){
    v->cap = (v->cap ? v->cap*2 : 8);
    v->data = (int*)realloc(v->data, v->cap*sizeof(int));
    if(!v->data){ perror("realloc"); exit(1); }
  }
  v->data[v->size++] = x;
}

// ------------------------------ ソート（ID昇順） ------------------------------
static int cmp_node_id (const void *a, const void *b){ const Node  *x=a, *y=b; return (x->id - y->id); }
static int cmp_tria3_id(const void *a, const void *b){ const Tria3 *x=a, *y=b; return (x->id - y->id); }
static int cmp_tria6_id(const void *a, const void *b){ const Tria6 *x=a, *y=b; return (x->id - y->id); }

// ------------------------------ 文字列・数値ユーティリティ ------------------------------
static void replace_nbsp_tabs(char *s){
  for(char *p=s; *p; ++p){
    unsigned char c=(unsigned char)*p;
    if(c==0xA0 || c=='\t'){ *p=' '; }
  }
}
static void rstrip(char *s){
  size_t n=strlen(s);
  while(n && (s[n-1]=='\r' || s[n-1]=='\n')){ s[--n]=0; }
}
static void strip_trailing_plus(char *s){
  size_t n=strlen(s);
  while(n && s[n-1]==' '){ n--; }
  if(n && s[n-1]=='+'){ s[n-1]=0; }
}

// "2.0694+8" → "2.0694E+8", "7.83-6" → "7.83E-6" に正規化
static char* normalize_number_stream(const char *src){
  size_t n=strlen(src);
  char *out=(char*)malloc(n*2+128);
  if(!out){ perror("malloc"); exit(1); }
  size_t j=0;
  for(size_t i=0;i<n;i++){
    char c=src[i];
    out[j++]=c;
    if(c=='.' && i+2<n){
      size_t k=i+1;
      int hasDigits=0;
      while(k<n && isdigit((unsigned char)src[k])){ hasDigits=1; k++; }
      if(hasDigits && k<n && (src[k]=='+' || src[k]=='-')){
        int prevE=(i>0 && (src[i-1]=='E'||src[i-1]=='e'));
        if(!prevE){ out[j++]='E'; }
      }
    }
  }
  out[j]=0;
  return out;
}

// 自由書式の行から数値のみ抽出
static int extract_numbers(const char *line, double *vals, int max){
  char *tmp=xstrdup(line);
  replace_nbsp_tabs(tmp);
  rstrip(tmp);
  strip_trailing_plus(tmp);
  char *fixed=normalize_number_stream(tmp);
  free(tmp);

  int cnt=0;
  char *p=fixed;
  while(*p && cnt<max){
    while(*p && !(isdigit((unsigned char)*p)||*p=='+'||*p=='-'||*p=='.'||*p=='E'||*p=='e')){
      p++;
    }
    if(!*p){ break; }
    char *endp=NULL;
    double v=strtod(p,&endp);
    if(endp==p){
      p++;
      continue;
    }
    vals[cnt++]=v;
    p=endp;
  }
  free(fixed);
  return cnt;
}

// カード名（先頭トークン）を飛ばしてデータ本体へ
static const char* after_head(const char *line){
  const char *p=line;
  while(*p==' '||*p=='\t'){ p++; }
  while(*p && *p!=' ' && *p!='\t' && *p!=','){ p++; }
  while(*p==' '||*p=='\t'||*p==','){ p++; }
  return p;
}

// 行先頭のカード名文字列を取り出す
static void head_token(const char *line, char out[32]){
  out[0]=0;
  char *tmp=xstrdup(line);
  replace_nbsp_tabs(tmp);
  char *p=tmp;
  while(*p==' '){ p++; }
  int k=0;
  while(*p && !isspace((unsigned char)*p) && *p!=',' && k<31){
    out[k++]=*p++;
  }
  out[k]=0;
  free(tmp);
}

// 連結トークン "0.2880007.8300-6" を "0.288000" / "7.8300-6" に分割（ヒューリスティクス）
static int split_concatenated_two_numbers(const char *tok, char left_out[64], char right_out[64]){
  int dot_count=0;
  size_t n=strlen(tok);
  for(size_t i=0;i<n;i++){
    if(tok[i]=='.'){ dot_count++; }
  }
  if(dot_count<2){
    return 0;
  }
  // 2つ目の '.' を探す
  size_t second_dot=0;
  int seen=0;
  for(size_t i=0;i<n;i++){
    if(tok[i]=='.'){
      if(seen==0){
        seen=1;
        continue;
      }
      second_dot=i;
      break;
    }
  }
  if(second_dot==0){
    return 0;
  }
  // 左側末尾の連続数字は右へ寄せる（例: "...2880007" の "7" は右側へ）
  size_t left_end=second_dot-1;
  while(left_end>0 && isdigit((unsigned char)tok[left_end])){
    left_end--;
  }
  size_t L=left_end+1;
  if(L>=64){ L=63; }
  memcpy(left_out, tok, L);
  left_out[L]='\0';

  size_t Rstart=left_end+1;
  char right_buf[128];
  size_t k=0;
  if(tok[Rstart]=='.'){
    right_buf[k++]='0'; // 先頭が '.' の場合は "0." に補正
  }
  for(size_t i=Rstart;i<n && k<sizeof(right_buf)-1;i++){
    right_buf[k++]=tok[i];
  }
  right_buf[k]='\0';

  // 安全な短縮コピー（必ず終端される）
  snprintf(right_out, 64, "%s", right_buf);
  return 2;
}

// 固定幅フィールド → double / int
static double field_to_double(const char *s, int n){
  char buf[64];
  int i=0;
  int j=n-1;
  int k=0;
  while(i<n && (s[i]==' '||s[i]=='\t')){ i++; }
  while(j>=i && (s[j]==' '||s[j]=='\t'||s[j]=='\r'||s[j]=='\n')){ j--; }
  for(int p=i; p<=j && k<(int)sizeof(buf)-1; ++p){
    buf[k++]=s[p];
  }
  buf[k]='\0';
  char *fixed=normalize_number_stream(buf);
  char *endp=NULL;
  double v=strtod(fixed,&endp);
  free(fixed);
  return v;
}
static int field_to_int(const char *s, int n){
  double v=field_to_double(s,n);
  return (int)llround(v);
}

// 自作：有限判定（isfinite 代替）
static int is_finite(double x){
  return (x == x) && (x != INFINITY) && (x != -INFINITY);
}

// ------------------------------ 検出ユーティリティ ------------------------------
static int detect_mesh_collector(FILE *fp, char *name, size_t cap){
  long pos=ftell(fp);
  char line[4096];
  int found=0;
  while(fgets(line,sizeof(line),fp)){
    if(strstr(line,"Mesh Collector:")!=NULL){
      const char *p=strstr(line,"Mesh Collector:");
      p += strlen("Mesh Collector:");
      while(*p==' ' || *p=='\t'){
        p++;
      }
      char buf[256];
      size_t j=0;
      while(*p && *p!='\r' && *p!='\n' && j<sizeof(buf)-1){
        buf[j++]=*p++;
      }
      buf[j]=0;
      while(j && buf[j-1]==' '){
        buf[--j]=0;
      }
      snprintf(name,cap,"%s",buf);
      found=1;
      break;
    }
  }
  fseek(fp,pos,SEEK_SET);
  return found;
}

static void detect_labels(FILE *fp, char *spc_label, size_t spc_cap, char *force_label, size_t force_cap){
  long pos=ftell(fp);
  char line[4096];
  while(fgets(line,sizeof(line),fp)){
    if(strstr(line,"Constraint:")!=NULL){
      const char *p=strstr(line,"Constraint:");
      p += strlen("Constraint:");
      while(*p==' ' || *p=='\t'){
        p++;
      }
      snprintf(spc_label, spc_cap, "%s", p);
      for(size_t i=strlen(spc_label); i>0; --i){
        if(spc_label[i-1]=='\r' || spc_label[i-1]=='\n'){
          spc_label[i-1]=0;
        }else{
          break;
        }
      }
    }
    if(strstr(line,"Load:")!=NULL){
      const char *p=strstr(line,"Load:");
      p += strlen("Load:");
      while(*p==' ' || *p=='\t'){
        p++;
      }
      snprintf(force_label, force_cap, "%s", p);
      for(size_t i=strlen(force_label); i>0; --i){
        if(force_label[i-1]=='\r' || force_label[i-1]=='\n'){
          force_label[i-1]=0;
        }else{
          break;
        }
      }
    }
  }
  fseek(fp,pos,SEEK_SET);
}

// 材料バナー（例: "$* Material: SteelLike::..."）から材料名を拾う（dumpで表示）
static char g_material_name[128]={0};
static void detect_material_name(FILE *fp){
  long pos=ftell(fp);
  char line[4096];
  while(fgets(line,sizeof(line),fp)){
    if(strstr(line,"Material:")!=NULL){
      const char *p=strstr(line,"Material:");
      p += strlen("Material:");
      while(*p==' ' || *p=='\t'){
        p++;
      }
      char buf[256];
      size_t j=0;
      while(*p && *p!=':' && *p!='\r' && *p!='\n' && j<sizeof(buf)-1){
        buf[j++]=*p++;
      }
      buf[j]=0;
      while(j && buf[j-1]==' '){
        buf[--j]=0;
      }
      snprintf(g_material_name,sizeof(g_material_name),"%s",buf);
      break;
    }
  }
  fseek(fp,pos,SEEK_SET);
}

// ------------------------------ mkdir ユーティリティ ------------------------------
static void make_output_dirs(const char *root, const char *part){
  char path[1024];
  snprintf(path,sizeof(path),"%s",root);
  MKDIR(path);
  snprintf(path,sizeof(path),"%s/%s",root,part);
  MKDIR(path);
  snprintf(path,sizeof(path),"%s/%s/mesh",root,part);
  MKDIR(path);
  snprintf(path,sizeof(path),"%s/%s/material",root,part);
  MKDIR(path);
  snprintf(path,sizeof(path),"%s/%s/Boundary Conditions",root,part);
  MKDIR(path);
  snprintf(path,sizeof(path),"%s/%s/debug",root,part);
  MKDIR(path);
}

// ------------------------------ 汎用ヘルパー ------------------------------
static int find_node_index_by_id(const NodeVec *v, int id){
  int lo=0;
  int hi=(int)v->size-1;
  while(lo<=hi){
    int mid=(lo+hi)/2;
    int key=v->data[mid].id;
    if(key==id){
      return mid;
    }
    if(key<id){
      lo = mid + 1;
    }else{
      hi = mid - 1;
    }
  }
  return -1;
}

static double tri_area_signed(const Node *a, const Node *b, const Node *c){
  // XY平面の符号付き面積（右手系）
  double ax = b->x - a->x;
  double ay = b->y - a->y;
  double bx = c->x - a->x;
  double by = c->y - a->y;
  return 0.5 * (ax*by - ay*bx);
}

static void normalize_vec3(double v[3]){
  double n = sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
  if(n>0.0){
    v[0]/=n; v[1]/=n; v[2]/=n;
  }
}

static int split_fields_simple(const char *s, char fields[][64], int max){
  int count=0;
  const char *p=s;
  while(*p && count<max){
    while(*p==' ' || *p=='\t' || *p==','){
      p++;
    }
    if(!*p){
      break;
    }
    const char *start=p;
    while(*p && *p!=' ' && *p!='\t' && *p!=',' && *p!='\r' && *p!='\n'){
      p++;
    }
    size_t len=(size_t)(p-start);
    if(len >= sizeof(fields[0])){
      len = sizeof(fields[0]) - 1;
    }
    memcpy(fields[count],start,len);
    fields[count][len]='\0';
    count++;
  }
  return count;
}

// ------------------------------ グローバル状態 ------------------------------
static int unitsys_mnmm = 0;  // PARAM,UNITSYS MN-MM を見つけたら 1 にする
// ------------------------------ PSHELL 検索（トップレベル関数） ------------------------------
static const PShell* find_pshell(const PShellVec *v, int pid){
  for(size_t i=0;i<v->size;i++){
    if(v->data[i].pid == pid){
      return &v->data[i];
    }
  }
  return NULL;
}

// ------------------------------ パーサ（MAT1/PSHELL/SPC/FORCE） ------------------------------
// MAT1：小フィールド（8桁固定）／大フィールド（16桁固定）／自由書式＋連結分割に対応
static int parse_mat1_line(const char *line, Mat1 *out){
  out->mid=0;
  out->hasE=0;
  out->hasNu=0;
  out->hasRho=0;
  out->E=0.0;
  out->nu=0.0;
  out->rho=0.0;

  char head[32];
  head_token(line, head);

  if(strncmp(head,"MAT1*",5)==0){
    // 大フィールド（16桁固定）
    char line80[81];
    memset(line80,' ',80);
    line80[80]='\0';
    size_t L=strlen(line);
    if(L>80){ L=80; }
    memcpy(line80,line,L);
    // 9–24 MID, 25–40 E, 41–56 G, 57–72 NU
    const char *f2=line80+8;
    const char *f3=line80+24;
    const char *f4=line80+40;
    const char *f5=line80+56;
    out->mid = field_to_int(f2,16);
    out->E   = field_to_double(f3,16);
    (void)f4; // G は今は未使用
    out->nu  = field_to_double(f5,16);
    out->hasE  = 1;
    out->hasNu = 1;

  }else if(strncmp(head,"MAT1",4)==0){
    // 小フィールド（8桁固定）："MAT1 22.0694+8 0.2880007.8300-6" など
    char line80[81];
    memset(line80,' ',80);
    line80[80]='\0';
    size_t L=strlen(line);
    if(L>80){ L=80; }
    memcpy(line80,line,L);
    const char *f2=line80+ 8; // MID（空白なら省略）
    const char *f3=line80+16; // E
    const char *f4=line80+24; // G（未使用可）
    const char *f5=line80+32; // ν
    const char *f6=line80+40; // ρ

    int blank_mid=1;
    for(int i=0;i<8;i++){
      if(f2[i]!=' ' && f2[i]!='\t'){
        blank_mid=0;
        break;
      }
    }
    out->mid = blank_mid ? 1 : field_to_int(f2,8);
    out->E   = field_to_double(f3,8);
    (void)f4;
    out->nu  = field_to_double(f5,8);
    out->rho = field_to_double(f6,8);
    out->hasE=1;
    out->hasNu=1;
    out->hasRho=1;

  }else{
    // 自由書式＋連結分割（νとρが1トークンにくっつくケースを想定）
    const char *body=after_head(line);

    // トークンに粗く分割（空白/カンマ）
    char tok[8][128];
    int tn=0;
    {
      const char *p=body;
      while(*p && tn<8){
        while(*p==' ' || *p=='\t' || *p==','){
          p++;
        }
        if(!*p){
          break;
        }
        const char *st=p;
        while(*p && *p!=' ' && *p!='\t' && *p!=',' && *p!='\r' && *p!='\n'){
          p++;
        }
        size_t len=(size_t)(p-st);
        if(len >= sizeof(tok[tn])){
          len = sizeof(tok[tn]) - 1;
        }
        memcpy(tok[tn],st,len);
        tok[tn][len]='\0';
        tn++;
      }
    }

    double vals[4];
    int vc=0;

    // 1トークン目：E
    if(tn>=1){
      char *fix=normalize_number_stream(tok[0]);
      vals[vc++]=strtod(fix,NULL);
      free(fix);
    }

    // 2トークン目：ν／ρ、または連結
    if(tn>=2){
      char left_tok[64];
      char right_tok[64];
      int sp=split_concatenated_two_numbers(tok[1], left_tok, right_tok);
      if(sp==2){
        char *fixL=normalize_number_stream(left_tok);
        char *fixR=normalize_number_stream(right_tok);
        vals[vc++]=strtod(fixL,NULL); // ν
        vals[vc++]=strtod(fixR,NULL); // ρ
        free(fixL);
        free(fixR);
      }else{
        char *fix=normalize_number_stream(tok[1]); // ν
        vals[vc++]=strtod(fix,NULL);
        free(fix);
        if(tn>=3){
          char *fix2=normalize_number_stream(tok[2]); // ρ
          vals[vc++]=strtod(fix2,NULL);
          free(fix2);
        }
      }
    }

    out->mid = 1;  // MID省略時の既定
    if(vc>=1){ out->E   = vals[0]; out->hasE  = 1; }
    if(vc>=2){ out->nu  = vals[1]; out->hasNu = 1; }
    if(vc>=3){ out->rho = vals[2]; out->hasRho= 1; }
  }

  // E を N/mm^2 に統一
  // UNITSYS=MN-MM の場合: mN/mm^2 → N/mm^2（÷1000）
  // それ以外は Pa とみなし: Pa → N/mm^2（×1e-6）
  if(out->hasE){
    if(unitsys_mnmm){
      out->E *= 1e-3;
    }else{
      out->E *= 1e-6;
    }
  }

  // 非有限値はユーザー指定の既定値（SteelLike）にフォールバック
  if(!is_finite(out->E) || !is_finite(out->nu) || !is_finite(out->rho)){
    out->E   = 220694.0;        // 220694 MPa → 220694 N/mm^2
    out->nu  = 0.288;
    out->rho = 7.83e-6;         // kg/mm^3
    out->hasE=1;
    out->hasNu=1;
    out->hasRho=1;
  }
  return out->hasE || out->hasNu || out->hasRho;
}

// PSHELL：小フィールド（8桁固定）＆自由書式（PID, MID1, T）
static int parse_pshell_line(const char *line, PShell *out){
  char head[32];
  head_token(line, head);

  if(strncmp(head,"PSHELL",6)==0){
    // 小フィールド（8桁固定）
    char line80[81];
    memset(line80,' ',80);
    line80[80]='\0';
    size_t L=strlen(line);
    if(L>80){ L=80; }
    memcpy(line80,line,L);

    // 8 桁ずつ（PID, MID1, T）
    const char *f2=line80+ 8; // PID
    const char *f3=line80+16; // MID1
    const char *f4=line80+24; // T（厚さ）
    out->pid  = field_to_int(f2,8);
    out->mid1 = field_to_int(f3,8);
    out->hasT = 1;
    out->T    = field_to_double(f4,8);
    return 1;
  }

  // フォールバック（自由書式）
  double vals[16];
  int n=extract_numbers(after_head(line), vals, 16);
  if(n>=3){
    out->pid  = (int)vals[0];
    out->mid1 = (int)vals[1];
    out->hasT = 1;
    out->T    = vals[2];
    return 1;
  }
  return 0;
}

// SPC（自由書式）
static int parse_spc_line(const char *line, SPCEntry *spc){
  char f[10][64];
  int nf=split_fields_simple(line, f, 10);
  if(nf<4){
    return 0;
  }
  spc->sid = atoi(f[1]);
  spc->gid = atoi(f[2]);
  strncpy(spc->comp, f[3], sizeof(spc->comp)-1);
  spc->comp[sizeof(spc->comp)-1]=0;
  spc->d = (nf>=5 ? atof(f[4]) : 0.0);
  return 1;
}

// FORCE（自由書式）
static int parse_force_line(const char *line, ForceEntry *fc){
  char *tmp=xstrdup(line);
  replace_nbsp_tabs(tmp);

  // 固定幅（8桁）FORCE を優先して解釈
  {
    char line80[81];
    memset(line80,' ',80);
    line80[80]='\0';
    size_t L=strlen(tmp);
    if(L>80){ L=80; }
    memcpy(line80,tmp,L);

    const char *f2=line80+ 8; // SID
    const char *f3=line80+16; // G
    const char *f4=line80+24; // CID
    const char *f5=line80+32; // F
    const char *f6=line80+40; // N1
    const char *f7=line80+48; // N2
    const char *f8=line80+56; // N3

    int sid = field_to_int(f2,8);
    int gid = field_to_int(f3,8);
    if(sid>0 && gid>0){
      fc->sid = sid;
      fc->gid = gid;
      fc->cid = field_to_int(f4,8);
      fc->F   = field_to_double(f5,8);
      fc->n1  = field_to_double(f6,8);
      fc->n2  = field_to_double(f7,8);
      fc->n3  = field_to_double(f8,8);
      free(tmp);
      return 1;
    }
  }

  char f[12][64];
  int nf=split_fields_simple(tmp, f, 12);
  if(nf>=8){
    fc->sid = atoi(f[1]);
    fc->gid = atoi(f[2]);
    fc->cid = atoi(f[3]);
    fc->F   = atof(f[4]);
    fc->n1  = atof(f[5]);
    fc->n2  = atof(f[6]);
    fc->n3  = atof(f[7]);
    free(tmp);
    return 1;
  }

  // "FORCE sid gid F n1n2 n3" 形式（n1/n2が連結）のフォールバック
  if(nf==6){
    fc->sid = atoi(f[1]);
    fc->gid = atoi(f[2]);
    fc->cid = 0;
    fc->F   = atof(f[3]);

    char left_tok[64]={0};
    char right_tok[64]={0};
    int split_pos = -1;
    size_t len=strlen(f[4]);
    for(size_t i=1;i<len;i++){
      if(f[4][i]=='+' || f[4][i]=='-'){
        split_pos=(int)i;
        break;
      }
    }
    if(split_pos>0){
      size_t L=(size_t)split_pos;
      if(L>=sizeof(left_tok)){ L=sizeof(left_tok)-1; }
      memcpy(left_tok, f[4], L);
      left_tok[L]='\0';
      snprintf(right_tok, sizeof(right_tok), "%s", f[4]+split_pos);
    }else{
      int sp=split_concatenated_two_numbers(f[4], left_tok, right_tok);
      if(sp!=2){
        left_tok[0]=0;
        right_tok[0]=0;
      }
    }

    if(left_tok[0] && right_tok[0]){
      char *fixL=normalize_number_stream(left_tok);
      char *fixR=normalize_number_stream(right_tok);
      fc->n1 = strtod(fixL,NULL);
      fc->n2 = strtod(fixR,NULL);
      free(fixL);
      free(fixR);
    }else{
      fc->n1 = atof(f[4]);
      fc->n2 = 0.0;
    }
    fc->n3 = atof(f[5]);
    free(tmp);
    return 1;
  }

  // 固定幅や連結数値に備えたフォールバック
  double vals[16];
  int n=extract_numbers(after_head(tmp), vals, 16);
  if(n>=6){
    fc->sid = (int)vals[0];
    fc->gid = (int)vals[1];
    if(n>=7){
      fc->cid = (int)vals[2];
      fc->F   = vals[3];
      fc->n1  = vals[4];
      fc->n2  = vals[5];
      fc->n3  = vals[6];
    }else{
      fc->cid = 0;
      fc->F   = vals[2];
      fc->n1  = vals[3];
      fc->n2  = vals[4];
      fc->n3  = vals[5];
    }
    free(tmp);
    return 1;
  }
  free(tmp);
  return 0;
}

// ------------------------------ 品質チェック ------------------------------
static int is_degenerate_tria3_ids(int a, int b, int c){
  if(a==0 || b==0 || c==0){
    return 1;
  }
  if(a==b || b==c || c==a){
    return 1;
  }
  return 0;
}

// ------------------------------ main ------------------------------
int main(int argc, char **argv){
  if(argc < 3){
    fprintf(stderr,
      "Usage: %s <input_bdf> <out_root> [part_name] [--part=<name>] [--dofnames] [--dump] [--plane=xz|xy]\n",
      argv[0]);
    return 1;
  }
  const char *infile  = argv[1];
  const char *outroot = argv[2];

  // オプション
  int  opt_dofnames = 0;
  int  opt_dump     = 0;
  int  opt_plane_xz = 0;
  int  opt_plane_xy = 0;
  char opt_part[256]={0};

  for(int ai=3; ai<argc; ++ai){
    if(strcmp(argv[ai], "--dofnames")==0){
      opt_dofnames = 1;
    }else if(strcmp(argv[ai], "--dump")==0){
      opt_dump = 1;
    }else if(strcmp(argv[ai], "--plane=xz")==0){
      opt_plane_xz = 1;
    }else if(strcmp(argv[ai], "--plane=xy")==0){
      opt_plane_xy = 1;
    }else if(strncmp(argv[ai], "--part=", 7)==0){
      snprintf(opt_part, sizeof(opt_part), "%s", argv[ai]+7);
    }
  }

  // part 名決定（CLI優先 → 第3引数 → Mesh Collector → 既定）
  char part[256];
  if(opt_part[0]){
    snprintf(part, sizeof(part), "%s", opt_part);
  }else if(argc>=4 && argv[3][0] != '-'){
    snprintf(part, sizeof(part), "%s", argv[3]);
  }else{
    FILE *tmp=fopen(infile, "rb");
    if(!tmp){
      perror("fopen");
      return 1;
    }
    if(!detect_mesh_collector(tmp, part, sizeof(part))){
      strcpy(part, "part_0001");
    }
    fclose(tmp);
  }

  // 出力ディレクトリ作成（debug含む）
  make_output_dirs(outroot, part);

  // x-z平面なら警告ファイル（mesh直下）
  if(opt_plane_xz){
    char warn_path[1024];
    snprintf(warn_path, sizeof(warn_path), "%s/%s/mesh/warning_plane.txt", outroot, part);
    FILE *fw=fopen(warn_path, "wb");
    if(fw){
      fprintf(fw, "x-z平面の可能性があります\n");
      fclose(fw);
    }else{
      perror("fopen(warning_plane.txt)");
    }
  }

  // dumpファイル（--dump時のみ）
  FILE *flog = NULL;
  if(opt_dump){
    char path_log[1024];
    snprintf(path_log, sizeof(path_log), "%s/%s/debug/parse_dump.txt", outroot, part);
    flog=fopen(path_log, "wb");
    if(!flog){
      perror("fopen(parse_dump.txt)");
    }else{
      fprintf(flog, "# Parse dump (input=%s, part=%s)\n\n", infile, part);
    }
  }

  // 入力ファイルを開く
  FILE *fp=fopen(infile, "rb");
  if(!fp){
    perror("fopen");
    return 1;
  }

  // 事前検出（ラベル・材料名）
  char spc_label[256]="";
  char force_label[256]="";
  detect_labels(fp, spc_label, sizeof(spc_label), force_label, sizeof(force_label));
  detect_material_name(fp);

  // 主要コンテナ
  NodeVec  nodes;  nodevec_init(&nodes);
  Tria3Vec t3s;    tria3vec_init(&t3s);
  Tria6Vec t6s;    tria6vec_init(&t6s);

  Mat1 mat=(Mat1){0};
  int  mat_found=0;

  PShellVec pshells; pshellvec_init(&pshells);
  SPCVec spcs;       spcvec_init(&spcs);
  ForceVec forces;   forcevec_init(&forces);

  int z_nonzero_detected = 0;

  // 行ごとに読み取り
  char line[4096];
  while(fgets(line, sizeof(line), fp)){
    char head[32];
    head_token(line, head);

    // コメント・空行
    if(head[0]==0 || head[0]=='$'){
      if(strstr(line, "PARAM UNITSYS MN-MM")){
        unitsys_mnmm = 1;
      }
      continue;
    }
    // PARAM,UNITSYS（自由書式）も拾う
    if(strstr(line, "PARAM") && strstr(line, "UNITSYS") && strstr(line, "MN-MM")){
      unitsys_mnmm = 1;
      continue;
    }

    // -------- GRID*（ラージ固定幅; 80桁） --------
    if(strcmp(head, "GRID*")==0){
      char line80[81];
      memset(line80, ' ', 80);
      line80[80] = '\0';

      size_t L=strlen(line);
      if(L>80){ L=80; }
      memcpy(line80, line, L);

      // 9–24 ID, 41–56 X, 57–72 Y（25–40 CP は今回は未使用）
      const char *f2 = line80 + 8;   // ID
      const char *f4 = line80 + 40;  // X
      const char *f5 = line80 + 56;  // Y

      Node nd={0};
      nd.id = field_to_int(f2,16);
      nd.x  = field_to_double(f4,16);
      nd.y  = field_to_double(f5,16);
      nd.z  = 0.0;

      // 継続 '*' 行から Z を取得
      while(1){
        long pos_before = ftell(fp);
        char cont[4096]={0};
        if(!fgets(cont, sizeof(cont), fp)){
          break;
        }
        char *s=cont;
        while(*s==' ' || *s=='\t'){
          s++;
        }
        if(*s!='*'){
          fseek(fp, pos_before, SEEK_SET);
          break;
        }
        char c80[81];
        memset(c80, ' ', 80);
        c80[80]='\0';
        size_t CL=strlen(cont);
        if(CL>80){ CL=80; }
        memcpy(c80, cont, CL);

        const char *c2 = c80 + 8;   // Z
        nd.z = field_to_double(c2,16);
      }

      // 極小値丸め
      const double EPS=1e-12;
      if(fabs(nd.x)<EPS){ nd.x=0.0; }
      if(fabs(nd.y)<EPS){ nd.y=0.0; }
      if(fabs(nd.z)<EPS){ nd.z=0.0; }

      // 平面指定：xzならXYへ写像（y<-z, z<-y）
      if(opt_plane_xz){
        double x=nd.x;
        double y=nd.y;
        double z=nd.z;
        nd.x=x;
        nd.y=z;
        nd.z=y;
      }else if(opt_plane_xy){
        if(fabs(nd.z)<EPS){
          nd.z=0.0;
        }
      }

      if(fabs(nd.z)>1e-12){
        z_nonzero_detected = 1;
      }

      if(flog){
        fprintf(flog, "[GRID*] LARGE-FIXED raw: %s", line);
        fprintf(flog, " chosen XYZ: (%.10f, %.10f, %.10f)\n\n", nd.x, nd.y, nd.z);
      }
      nodevec_push(&nodes, nd);
      continue;
    }

    // -------- GRID（自由書式） --------
    if(strcmp(head, "GRID")==0){
      double v[16];
      int n=extract_numbers(line, v, 16);
      Node nd={0};
      if(n>=6){
        nd.id=(int)v[0]; // cp=v[1]（未使用）
        nd.x=v[2];
        nd.y=v[3];
        nd.z=v[4];
      }else if(n==5){
        nd.id=(int)v[0];
        nd.x=v[1];
        nd.y=v[2];
        nd.z=v[3];
      }else{
        continue;
      }

      const double EPS=1e-12;
      if(fabs(nd.x)<EPS){ nd.x=0.0; }
      if(fabs(nd.y)<EPS){ nd.y=0.0; }
      if(fabs(nd.z)<EPS){ nd.z=0.0; }

      if(opt_plane_xz){
        double x=nd.x;
        double y=nd.y;
        double z=nd.z;
        nd.x=x;
        nd.y=z;
        nd.z=y;
      }else if(opt_plane_xy){
        if(fabs(nd.z)<EPS){
          nd.z=0.0;
        }
      }

      if(fabs(nd.z)>1e-12){
        z_nonzero_detected=1;
      }

      if(flog){
        fprintf(flog, "[GRID ] raw: %s", line);
        fprintf(flog, " chosen XYZ: (%.10f, %.10f, %.10f)\n\n", nd.x, nd.y, nd.z);
      }
      nodevec_push(&nodes, nd);
      continue;
    }

    // -------- CTRIA3（自由書式; カード名スキップ） --------
    if(strncmp(head, "CTRIA3", 6)==0){
      double vals[16];
      int n=extract_numbers(after_head(line), vals, 16);
      if(n>=5){
        Tria3 e;
        e.id =(int)vals[0];
        e.pid=(int)vals[1];
        e.n1 =(int)vals[2];
        e.n2 =(int)vals[3];
        e.n3 =(int)vals[4];
        tria3vec_push(&t3s, e);
      }
      continue;
    }
   // -------- CTRIA6（自由書式; カード名スキップ） --------
    if(strncmp(head, "CTRIA6", 6)==0){
      double vals[32];
      int n=extract_numbers(after_head(line), vals, 32);
      if(n>=8){
        Tria6 e;
        e.id =(int)vals[0];
        e.pid=(int)vals[1];
        e.n1 =(int)vals[2];
        e.n2 =(int)vals[3];
        e.n3 =(int)vals[4];
        e.n4 =(int)vals[5];
        e.n5 =(int)vals[6];
        e.n6 =(int)vals[7];
        tria6vec_push(&t6s, e);
      }
      continue;
    }

    // -------- PSHELL --------
    if(strncmp(head, "PSHELL", 6)==0){
      PShell p={0};
      if(parse_pshell_line(line, &p)){
        pshellvec_push(&pshells, p);
      }
      continue;
    }

    // -------- MAT1 --------
    if(strncmp(head, "MAT1", 4)==0){
      if(parse_mat1_line(line, &mat)){
        mat_found=1;
      }
      continue;
    }

    // -------- SPC --------
    if(strncmp(head, "SPC", 3)==0){
      SPCEntry s={0};
      if(parse_spc_line(line, &s)){
        spcvec_push(&spcs, s);
      }
      continue;
    }

    // -------- FORCE --------
    if(strncmp(head, "FORCE", 5)==0){
      ForceEntry fce={0};
      if(parse_force_line(line, &fce)){
        forcevec_push(&forces, fce);
      }
      continue;
    }
  } // while fgets
  fclose(fp);


  // ファイル読み取り完了


  // ID順に並べ替え（内部ノードID=1..N に対応）
  qsort(nodes.data, nodes.size, sizeof(Node),  cmp_node_id);
  qsort(t3s.data,   t3s.size,   sizeof(Tria3), cmp_tria3_id);
  qsort(t6s.data,   t6s.size,   sizeof(Tria6), cmp_tria6_id);

  // -------- mesh/mesh.dat --------
  char path_mesh[1024];
  snprintf(path_mesh, sizeof(path_mesh), "%s/%s/mesh/mesh.dat", outroot, part);
  FILE *fmesh=fopen(path_mesh, "wb");
  if(!fmesh){
    perror("fopen(mesh.dat)");
    return 1;
  }

  int has_tria3 = ((int)t3s.size > 0);
  int has_tria6 = ((int)t6s.size > 0);

  fprintf(fmesh, "Total number of nodes [–]\n%zu\n", nodes.size);
  fprintf(fmesh, "Total number of elements [–]\n%zu\n", t3s.size + t6s.size);
  fprintf(fmesh, "Element type\n");
  if(has_tria3 && !has_tria6){
    fprintf(fmesh, "CTRIA3\n");
  }else if(!has_tria3 && has_tria6){
    fprintf(fmesh, "CTRIA6\n");
  }else if(has_tria3 && has_tria6){
    fprintf(fmesh, "Multiple element types (CTRIA3/CTRIA6) present. Please check input.\n");
  }else{
    fprintf(fmesh, "None\n");
  }

  if(z_nonzero_detected){
    fprintf(fmesh, "Dimension check\n");
    fprintf(fmesh, "Detected non–2D coordinates (non–zero Z).\n");
  }

  // CTRIA3 退化事前チェック（0/重複ノードはスキップ）
  size_t bad_tria3_count = 0;
  int   *bad_eids        = NULL;
  size_t bad_cap         = 0;

  for(size_t i=0;i<t3s.size;i++){
    int ids[3] = {t3s.data[i].n1, t3s.data[i].n2, t3s.data[i].n3};
    int in[3];
    for(int k=0;k<3;k++){
      int idx=find_node_index_by_id(&nodes, ids[k]);
      in[k] = (idx<0 ? 0 : (idx+1));
    }
    if(is_degenerate_tria3_ids(in[0], in[1], in[2])){
      if(bad_tria3_count == bad_cap){
        bad_cap = (bad_cap ? bad_cap*2 : 32);
        bad_eids = (int*)realloc(bad_eids, bad_cap*sizeof(int));
        if(!bad_eids){
          perror("realloc");
          exit(1);
        }
      }
      bad_eids[bad_tria3_count++] = t3s.data[i].id;
    }
  }

  if(bad_tria3_count > 0){
    fprintf(fmesh, "Element quality check\n");
    fprintf(fmesh, "Detected degenerate CTRIA3 elements (skipped): %zu\n", bad_tria3_count);
    fprintf(fmesh, "EIDs: ");
    for(size_t i=0;i<bad_tria3_count;i++){
      fprintf(fmesh, "%d%s", bad_eids[i], (i+1<bad_tria3_count) ? ", " : "\n");
    }
  }
  free(bad_eids);

  // ノード座標（内部ID=1..N）
  fprintf(fmesh, "nodes\n");
  for(size_t i=0;i<nodes.size;i++){
    fprintf(fmesh, "%zu, %.10f, %.10f, %.10f\n",
            i+1, nodes.data[i].x, nodes.data[i].y, nodes.data[i].z);
  }

  // 要素接続（入力の節点番号 → 内部ノード番号）
  fprintf(fmesh, "elements\n");
  size_t ecount=0;

  // surface/ridgeline 準備
  SurfElemVec se; surfelemvec_init(&se);
  EdgeVec edges; edgevec_init(&edges);
  int is2d = !z_nonzero_detected;

  // CTRIA3（退化はスキップ）
  for(size_t i=0;i<t3s.size;i++){
    int ids[3] = {t3s.data[i].n1, t3s.data[i].n2, t3s.data[i].n3};
    int in[3];
    for(int k=0;k<3;k++){
      int idx=find_node_index_by_id(&nodes, ids[k]);
      in[k] = (idx<0 ? 0 : (idx+1));
    }
    if(is_degenerate_tria3_ids(in[0], in[1], in[2])){
      if(flog){
        fprintf(flog,
          "[CTRIA3] EID=%d PID=%d G=(%d,%d,%d) -> internal (%d,%d,%d) : DEGENERATE\n",
          t3s.data[i].id, t3s.data[i].pid, ids[0], ids[1], ids[2], in[0], in[1], in[2]);
      }
      continue;
    }
    size_t internal_eid = ++ecount;
    if(flog){
      fprintf(flog,
        "[CTRIA3] EID=%d PID=%d G=(%d,%d,%d) -> internal (%d,%d,%d) : OK\n",
        t3s.data[i].id, t3s.data[i].pid, ids[0], ids[1], ids[2], in[0], in[1], in[2]);
    }
    fprintf(fmesh, "%zu, %d, %d, %d\n", internal_eid, in[0], in[1], in[2]);

    if(in[0]>0 && in[1]>0 && in[2]>0){
      SurfElem e;
      e.eid = (int)internal_eid;
      e.n[0] = in[0]-1;
      e.n[1] = in[1]-1;
      e.n[2] = in[2]-1;
      if(is2d){
        e.normal[0]=0.0; e.normal[1]=0.0; e.normal[2]=1.0;
      }else{
        const Node *A=&nodes.data[e.n[0]];
        const Node *B=&nodes.data[e.n[1]];
        const Node *C=&nodes.data[e.n[2]];
        double ab[3]={B->x-A->x, B->y-A->y, B->z-A->z};
        double ac[3]={C->x-A->x, C->y-A->y, C->z-A->z};
        e.normal[0]=ab[1]*ac[2]-ab[2]*ac[1];
        e.normal[1]=ab[2]*ac[0]-ab[0]*ac[2];
        e.normal[2]=ab[0]*ac[1]-ab[1]*ac[0];
        normalize_vec3(e.normal);
      }
      e.surface_id = 0;
      surfelemvec_push(&se, e);
      edgevec_add(&edges, e.n[0], e.n[1], (int)se.size-1);
      edgevec_add(&edges, e.n[1], e.n[2], (int)se.size-1);
      edgevec_add(&edges, e.n[2], e.n[0], (int)se.size-1);
    }
  }

  // CTRIA6（簡易：接続だけ出力。面積/慣性は将来拡張）
  for(size_t i=0;i<t6s.size;i++){
    int ids[6] = {t6s.data[i].n1, t6s.data[i].n2, t6s.data[i].n3,
                  t6s.data[i].n4, t6s.data[i].n5, t6s.data[i].n6};
    int in[6];
    for(int k=0;k<6;k++){
      int idx=find_node_index_by_id(&nodes, ids[k]);
      in[k] = (idx<0 ? 0 : (idx+1));
    }
    size_t internal_eid = ++ecount;
    fprintf(fmesh, "%zu, %d, %d, %d, %d, %d, %d\n",
            internal_eid, in[0], in[1], in[2], in[3], in[4], in[5]);

    if(in[0]>0 && in[1]>0 && in[2]>0){
      SurfElem e;
      e.eid = (int)internal_eid;
      e.n[0] = in[0]-1;
      e.n[1] = in[1]-1;
      e.n[2] = in[2]-1;
      if(is2d){
        e.normal[0]=0.0; e.normal[1]=0.0; e.normal[2]=1.0;
      }else{
        const Node *A=&nodes.data[e.n[0]];
        const Node *B=&nodes.data[e.n[1]];
        const Node *C=&nodes.data[e.n[2]];
        double ab[3]={B->x-A->x, B->y-A->y, B->z-A->z};
        double ac[3]={C->x-A->x, C->y-A->y, C->z-A->z};
        e.normal[0]=ab[1]*ac[2]-ab[2]*ac[1];
        e.normal[1]=ab[2]*ac[0]-ab[0]*ac[2];
        e.normal[2]=ab[0]*ac[1]-ab[1]*ac[0];
        normalize_vec3(e.normal);
      }
      e.surface_id = 0;
      surfelemvec_push(&se, e);
      edgevec_add(&edges, e.n[0], e.n[1], (int)se.size-1);
      edgevec_add(&edges, e.n[1], e.n[2], (int)se.size-1);
      edgevec_add(&edges, e.n[2], e.n[0], (int)se.size-1);
    }
  }
  fclose(fmesh);

  // -------- surface/ridgeline 分類 --------
  const double angle_deg = 60.0;
  const double cos_thresh = cos(angle_deg * (M_PI/180.0));

  if(se.size>0){
    if(is2d){
      for(size_t i=0;i<se.size;i++){
        se.data[i].surface_id = 1;
      }
    }else{
      int *parent = (int*)malloc(se.size*sizeof(int));
      if(!parent){ perror("malloc"); exit(1); }
      for(size_t i=0;i<se.size;i++){ parent[i]=(int)i; }
      #define FIND(x) ({int r=(x); while(parent[r]!=r) r=parent[r]; r;})
      #define UNION(a,b) do{int ra=FIND(a), rb=FIND(b); if(ra!=rb) parent[rb]=ra;}while(0)

      for(size_t i=0;i<edges.size;i++){
        EdgeRec *er=&edges.data[i];
        if(er->count==2 && er->e2>=0){
          SurfElem *a=&se.data[er->e1];
          SurfElem *b=&se.data[er->e2];
          double dot = a->normal[0]*b->normal[0]
                     + a->normal[1]*b->normal[1]
                     + a->normal[2]*b->normal[2];
          if(dot >= cos_thresh){
            UNION(er->e1, er->e2);
          }
        }
      }
      int *root_to_id = (int*)calloc(se.size, sizeof(int));
      if(!root_to_id){ perror("calloc"); exit(1); }
      int sid=0;
      for(size_t i=0;i<se.size;i++){
        int r=FIND((int)i);
        if(root_to_id[r]==0){
          root_to_id[r]=++sid;
        }
        se.data[i].surface_id = root_to_id[r];
      }
      free(root_to_id);
      free(parent);
      #undef FIND
      #undef UNION
    }
  }

  RidgeVec ridges; ridgevec_init(&ridges);
  for(size_t i=0;i<edges.size;i++){
    EdgeRec *er=&edges.data[i];
    int e1=er->e1;
    int e2=(er->count==2 ? er->e2 : -1);
    int s1 = (e1>=0 ? se.data[e1].surface_id : 0);
    int s2 = (e2>=0 ? se.data[e2].surface_id : 0);
    if(er->count==1 || (er->count==2 && s1!=s2)){
      RidgeEdge re;
      re.n1 = er->n1;
      re.n2 = er->n2;
      re.e1 = e1;
      re.e2 = e2;
      re.s1 = s1;
      re.s2 = s2;
      const Node *A=&nodes.data[re.n1];
      const Node *B=&nodes.data[re.n2];
      re.dir[0]=B->x-A->x;
      re.dir[1]=B->y-A->y;
      re.dir[2]=B->z-A->z;
      normalize_vec3(re.dir);
      re.rid = 0;
      ridgevec_push(&ridges, re);
    }
  }

  if(ridges.size>0){
    int *rparent = (int*)malloc(ridges.size*sizeof(int));
    if(!rparent){ perror("malloc"); exit(1); }
    for(size_t i=0;i<ridges.size;i++){ rparent[i]=(int)i; }
    #define RFIND(x) ({int r=(x); while(rparent[r]!=r) r=rparent[r]; r;})
    #define RUNION(a,b) do{int ra=RFIND(a), rb=RFIND(b); if(ra!=rb) rparent[rb]=ra;}while(0)

    IntVec *node_edges = (IntVec*)calloc(nodes.size, sizeof(IntVec));
    if(!node_edges){ perror("calloc"); exit(1); }
    for(size_t i=0;i<ridges.size;i++){
      intvec_push(&node_edges[ridges.data[i].n1], (int)i);
      intvec_push(&node_edges[ridges.data[i].n2], (int)i);
    }
    for(size_t n=0;n<nodes.size;n++){
      IntVec *lst=&node_edges[n];
      for(size_t i=0;i<lst->size;i++){
        for(size_t j=i+1;j<lst->size;j++){
          int a=lst->data[i];
          int b=lst->data[j];
          RidgeEdge *ea=&ridges.data[a];
          RidgeEdge *eb=&ridges.data[b];
          int pa1 = ea->s1<ea->s2 ? ea->s1 : ea->s2;
          int pa2 = ea->s1<ea->s2 ? ea->s2 : ea->s1;
          int pb1 = eb->s1<eb->s2 ? eb->s1 : eb->s2;
          int pb2 = eb->s1<eb->s2 ? eb->s2 : eb->s1;
          if(pa1!=pb1 || pa2!=pb2){
            continue;
          }
          double dot = fabs(ea->dir[0]*eb->dir[0]
                           +ea->dir[1]*eb->dir[1]
                           +ea->dir[2]*eb->dir[2]);
          if(dot >= cos_thresh){
            RUNION(a,b);
          }
        }
      }
    }
    int *root_to_id = (int*)calloc(ridges.size, sizeof(int));
    if(!root_to_id){ perror("calloc"); exit(1); }
    int rid=0;
    for(size_t i=0;i<ridges.size;i++){
      int r=RFIND((int)i);
      if(root_to_id[r]==0){
        root_to_id[r]=++rid;
      }
      ridges.data[i].rid = root_to_id[r];
    }
    for(size_t n=0;n<nodes.size;n++){
      free(node_edges[n].data);
    }
    free(node_edges);
    free(root_to_id);
    free(rparent);
    #undef RFIND
    #undef RUNION
  }

  // -------- mesh/surface.dat --------
  {
    char path_surface[1024];
    snprintf(path_surface, sizeof(path_surface), "%s/%s/mesh/surface.dat", outroot, part);
    FILE *fs=fopen(path_surface, "wb");
    if(fs){
      fprintf(fs, "angle=60\n");
      for(size_t i=0;i<se.size;i++){
        fprintf(fs, "%d %d\n", se.data[i].surface_id, se.data[i].eid);
      }
      fclose(fs);
    }else{
      perror("fopen(surface.dat)");
    }
  }

  // -------- mesh/ridgeline.dat --------
  {
    char path_ridge[1024];
    snprintf(path_ridge, sizeof(path_ridge), "%s/%s/mesh/ridgeline.dat", outroot, part);
    FILE *fr=fopen(path_ridge, "wb");
    if(fr){
      fprintf(fr, "angle=60\n");
      for(size_t i=0;i<ridges.size;i++){
        RidgeEdge *re=&ridges.data[i];
        if(re->e1>=0){
          fprintf(fr, "%d %d\n", re->rid, se.data[re->e1].eid);
        }
        if(re->e2>=0){
          fprintf(fr, "%d %d\n", re->rid, se.data[re->e2].eid);
        }
      }
      fclose(fr);
    }else{
      perror("fopen(ridgeline.dat)");
    }
  }

  // -------- material/material.dat --------
  char path_mat[1024];
  snprintf(path_mat, sizeof(path_mat), "%s/%s/material/material.dat", outroot, part);
  FILE *fmat=fopen(path_mat, "wb");
  if(!fmat){
    perror("fopen(material.dat)");
    return 1;
  }
  fprintf(fmat, "Young's modulus [N/mm^2]\n%.10g\n", mat_found ? mat.E : 0.0);
  fprintf(fmat, "Poisson's ratio [–]\n%.10g\n", (mat_found && mat.hasNu) ? mat.nu : 0.0);
  fprintf(fmat, "density [kg/mm^3]\n%.10g\n",   (mat_found && mat.hasRho)? mat.rho: 0.0);
  fclose(fmat);

  // -------- material/Volume.dat（PIDごと厚さ・材質反映） --------
  double total_volume_mm3 = 0.0;
  double total_mass_g     = 0.0;
  double cx_mass = 0.0;
  double cy_mass = 0.0;

  for(size_t i=0;i<t3s.size;i++){
    int i1 = find_node_index_by_id(&nodes, t3s.data[i].n1);
    int i2 = find_node_index_by_id(&nodes, t3s.data[i].n2);
    int i3 = find_node_index_by_id(&nodes, t3s.data[i].n3);
    if(i1<0 || i2<0 || i3<0){
      continue;
    }
    const Node *A=&nodes.data[i1];
    const Node *B=&nodes.data[i2];
    const Node *C=&nodes.data[i3];

    double area = fabs(tri_area_signed(A,B,C));

    const PShell *ps = find_pshell(&pshells, t3s.data[i].pid);
    double T = (ps && ps->hasT && ps->T>0.0) ? ps->T : 1.0;                     // 板厚
    double rho_gmm3 = (mat_found && mat.hasRho) ? (mat.rho * 1000.0) : 0.0;     // kg/mm^3 → g/mm^3

    double vol  = area * T;
    double mass = rho_gmm3 * vol;

    total_volume_mm3 += vol;
    total_mass_g     += mass;

    double cx_tri = (A->x + B->x + C->x) / 3.0;
    double cy_tri = (A->y + B->y + C->y) / 3.0;

    cx_mass += mass * cx_tri;
    cy_mass += mass * cy_tri;
  }

  char path_vol[1024];
  snprintf(path_vol, sizeof(path_vol), "%s/%s/material/Volume.dat", outroot, part);
  FILE *fvol=fopen(path_vol, "wb");
  if(!fvol){
    perror("fopen(Volume.dat)");
    return 1;
  }
  fprintf(fvol, "Volume [mm^3]\n%.10f\n", total_volume_mm3);
  fprintf(fvol, "mass [g]\n%.10f\n",       total_mass_g);
  fclose(fvol);

  // -------- material/Inertia.dat（COMまわり慣性） --------
  double Ix_origin = 0.0;
  double Iy_origin = 0.0;
  double Iz_origin = 0.0;

  for(size_t i=0;i<t3s.size;i++){
    int idx[3] = {
      find_node_index_by_id(&nodes, t3s.data[i].n1),
      find_node_index_by_id(&nodes, t3s.data[i].n2),
      find_node_index_by_id(&nodes, t3s.data[i].n3)
    };
    if(idx[0]<0 || idx[1]<0 || idx[2]<0){
      continue;
    }
    const Node *A=&nodes.data[idx[0]];
    const Node *B=&nodes.data[idx[1]];
    const Node *C=&nodes.data[idx[2]];
    const Node poly[3]={*A, *B, *C};

    // 多角形公式（三角形）
    double area2 = 0.0;
    double Ix_area = 0.0;
    double Iy_area = 0.0;
    double Iz_area = 0.0;
    for(int k=0;k<3;k++){
      const Node *P=&poly[k];
      const Node *Q=&poly[(k+1)%3];
      double cross = P->x * Q->y - Q->x * P->y;
      area2 += cross;
      Ix_area += cross * (P->y*P->y + P->y*Q->y + Q->y*Q->y);
      Iy_area += cross * (P->x*P->x + P->x*Q->x + Q->x*Q->x);
      Iz_area += cross * (P->x*P->x + P->x*Q->x + Q->x*Q->x
                        + P->y*P->y + P->y*Q->y + Q->y*Q->y);
    }

    const PShell *ps = find_pshell(&pshells, t3s.data[i].pid);
    double T = (ps && ps->hasT && ps->T>0.0) ? ps->T : 1.0;
    double rho_gmm3 = (mat_found && mat.hasRho) ? (mat.rho * 1000.0) : 0.0;

    double factor = (1.0/12.0) * T * rho_gmm3; // g
    Ix_origin += factor * fabs(Ix_area);
    Iy_origin += factor * fabs(Iy_area);
    Iz_origin += factor * fabs(Iz_area);
  }

  // COM（質量重み）
  double cx = (total_mass_g>0.0) ? (cx_mass / total_mass_g) : 0.0;
  double cy = (total_mass_g>0.0) ? (cy_mass / total_mass_g) : 0.0;

  // 並進軸の定理で COM へ移す
  double Ix_com = Ix_origin - total_mass_g * (cy * cy);
  double Iy_com = Iy_origin - total_mass_g * (cx * cx);
  double Iz_com = Iz_origin - total_mass_g * (cx * cx + cy * cy);

  char path_iner[1024];
  snprintf(path_iner, sizeof(path_iner), "%s/%s/material/Inertia.dat", outroot, part);
  FILE *finer=fopen(path_iner, "wb");
  if(!finer){
    perror("fopen(Inertia.dat)");
    return 1;
  }
  fprintf(finer, "Ix (about COM x-axis) [g·mm^2]\n%.10f\n", Ix_com);
  fprintf(finer, "Iy (about COM y-axis) [g·mm^2]\n%.10f\n", Iy_com);
  fprintf(finer, "Iz (about COM z-axis) [g·mm^2]\n%.10f\n", Iz_com);
  fclose(finer);

  // -------- Boundary Conditions/boundary.dat --------
  char path_bcdir[1024];
  snprintf(path_bcdir, sizeof(path_bcdir), "%s/%s/Boundary Conditions", outroot, part);
  char path_bc[1024];
  snprintf(path_bc, sizeof(path_bc), "%s/boundary.dat", path_bcdir);
  FILE *fbc=fopen(path_bc, "wb");
  if(!fbc){
    perror("fopen(boundary.dat)");
    return 1;
  }

  int *node_surface = (int*)calloc(nodes.size, sizeof(int));
  int *node_ridge   = (int*)calloc(nodes.size, sizeof(int));
  IntVec *node_ridges = NULL;
  if(is2d){
    node_ridges = (IntVec*)calloc(nodes.size, sizeof(IntVec));
  }
  if(!node_surface || !node_ridge || (is2d && !node_ridges)){
    perror("calloc");
    exit(1);
  }
  for(size_t i=0;i<se.size;i++){
    for(int k=0;k<3;k++){
      int n=se.data[i].n[k];
      if(n<0 || n>=(int)nodes.size){ continue; }
      if(node_surface[n]==0){
        node_surface[n]=se.data[i].surface_id;
      }else if(node_surface[n]!=se.data[i].surface_id){
        node_surface[n]=-1;
      }
    }
  }
  for(size_t i=0;i<ridges.size;i++){
    int rid=ridges.data[i].rid;
    int nn[2]={ridges.data[i].n1, ridges.data[i].n2};
    for(int k=0;k<2;k++){
      int n=nn[k];
      if(n<0 || n>=(int)nodes.size){ continue; }
      if(is2d){
        IntVec *lst=&node_ridges[n];
        int exists=0;
        for(size_t j=0;j<lst->size;j++){
          if(lst->data[j]==rid){ exists=1; break; }
        }
        if(!exists){
          intvec_push(lst, rid);
        }
      }else{
        if(node_ridge[n]==0){
          node_ridge[n]=rid;
        }else if(node_ridge[n]!=rid){
          node_ridge[n]=-1;
        }
      }
    }
  }
  for(size_t i=0;i<nodes.size;i++){
    if(node_surface[i]<0){ node_surface[i]=0; }
    if(node_ridge[i]<0){ node_ridge[i]=0; }
    if(is2d && node_ridges){
      if(node_ridges[i].size==1){
        node_ridge[i]=node_ridges[i].data[0];
      }else{
        node_ridge[i]=0;
      }
    }
  }

  typedef struct {
    int target_type; /* 1=node, 2=surface, 3=ridgeline */
    int id;
    char comp[8];
    double d;
  } FixLine;
  FixLine *fix_lines = NULL;
  size_t fix_lines_count = 0;
  size_t fix_lines_cap = 0;

  for(size_t i=0;i<spcs.size;i++){
    const SPCEntry *s=&spcs.data[i];
    if(s->gid<=0 || s->comp[0]==0){
      continue;
    }
    int nidx=find_node_index_by_id(&nodes, s->gid);
    const char *target="node";
    int tid=s->gid;
    if(nidx>=0){
      if(is2d){
        if(node_ridges && node_ridges[nidx].size==1){
          target="ridgeline";
          tid=node_ridges[nidx].data[0];
        }
      }else if(node_surface[nidx]>0){
        target="surface";
        tid=node_surface[nidx];
      }
    }
    int target_type = 1;
    if(strcmp(target, "surface") == 0){
      target_type = 2;
    }else if(strcmp(target, "ridgeline") == 0){
      target_type = 3;
    }
    int duplicate = 0;
    for(size_t j=0;j<fix_lines_count;j++){
      if(fix_lines[j].target_type == target_type &&
         fix_lines[j].id == tid &&
         strcmp(fix_lines[j].comp, s->comp) == 0 &&
         fabs(fix_lines[j].d - s->d) < 1e-12){
        duplicate = 1;
        break;
      }
    }
    if(duplicate){
      continue;
    }
    if(fix_lines_count == fix_lines_cap){
      size_t new_cap = fix_lines_cap ? fix_lines_cap * 2 : 32;
      FixLine *tmp = (FixLine*)realloc(fix_lines, new_cap * sizeof(FixLine));
      if(!tmp){
        perror("realloc");
        exit(1);
      }
      fix_lines = tmp;
      fix_lines_cap = new_cap;
    }
    fix_lines[fix_lines_count].target_type = target_type;
    fix_lines[fix_lines_count].id = tid;
    strncpy(fix_lines[fix_lines_count].comp, s->comp, sizeof(fix_lines[fix_lines_count].comp) - 1);
    fix_lines[fix_lines_count].comp[sizeof(fix_lines[fix_lines_count].comp) - 1] = '\0';
    fix_lines[fix_lines_count].d = s->d;
    fix_lines_count++;
  }
  size_t total_bc = fix_lines_count;
  for(size_t i=0;i<forces.size;i++){
    if(forces.data[i].gid>0){
      total_bc++;
    }
  }
  fprintf(fbc, "Total number of Boundary Conditions [–]\n%zu\n", total_bc);
  if(unitsys_mnmm){
    fprintf(fbc, "UNITSYS\nMN-MM\n");
  }

  // SPC 見出し
  if(spc_label[0]){
    fprintf(fbc, "%s\n", spc_label);
  }else{
    fprintf(fbc, "Constraint: SPC set\n");
  }
  fprintf(fbc, "Fix\n");
  for(size_t i=0;i<fix_lines_count;i++){
    const char *target = "node";
    if(fix_lines[i].target_type == 2){
      target = "surface";
    }else if(fix_lines[i].target_type == 3){
      target = "ridgeline";
    }
    fprintf(fbc, "%s %d %s %.6f\n",
            target,
            fix_lines[i].id,
            fix_lines[i].comp,
            fix_lines[i].d);
  }

  // FORCE 見出し
  if(force_label[0]){
    fprintf(fbc, "%s\n", force_label);
  }else{
    fprintf(fbc, "Load: FORCE set\n");
  }
  fprintf(fbc, "Force\n");

  for(size_t i=0;i<forces.size;i++){
    const ForceEntry *fc=&forces.data[i];
    if(fc->gid<=0){
      continue;
    }
    double F_out_N = unitsys_mnmm ? (fc->F / 1000.0) : fc->F; // mN -> N
    double fx = F_out_N * fc->n1;
    double fy = F_out_N * fc->n2;
    double fz = F_out_N * fc->n3;
    int axis = 0;
    double val = 0.0;
    double ax=fabs(fx), ay=fabs(fy), az=fabs(fz);
    if(ax>=ay && ax>=az){ axis=1; val=fx; }
    else if(ay>=ax && ay>=az){ axis=2; val=fy; }
    else if(az>=ax && az>=ay){ axis=3; val=fz; }

    int nidx=find_node_index_by_id(&nodes, fc->gid);
    const char *target="node";
    int tid=fc->gid;
    if(nidx>=0){
      if(is2d){
        if(node_ridges && node_ridges[nidx].size==1){
          target="ridgeline";
          tid=node_ridges[nidx].data[0];
        }
      }else{
        if(node_ridge[nidx]>0){
          target="ridgeline";
          tid=node_ridge[nidx];
        }else if(node_surface[nidx]>0){
          target="surface";
          tid=node_surface[nidx];
        }
      }
    }
    fprintf(fbc, "%s %d 123456 %d %.6g\n", target, tid, axis, val);
  }
  free(fix_lines);
  free(node_surface);
  free(node_ridge);
  if(node_ridges){
    for(size_t i=0;i<nodes.size;i++){
      free(node_ridges[i].data);
    }
    free(node_ridges);
  }
  fclose(fbc);

  // -------- dump：座標統計・材料名（可視化補助） --------
  if(flog){
    double xmin= 1e300;
    double xmax=-1e300;
    double ymin= 1e300;
    double ymax=-1e300;
    double zmin= 1e300;
    double zmax=-1e300;

    double xsum=0.0;
    double ysum=0.0;
    double zsum=0.0;

    for(size_t i=0;i<nodes.size;i++){
      double x=nodes.data[i].x;
      double y=nodes.data[i].y;
      double z=nodes.data[i].z;

      if(x < xmin){ xmin = x; }
      if(x > xmax){ xmax = x; }
      xsum += x;

      if(y < ymin){ ymin = y; }
      if(y > ymax){ ymax = y; }
      ysum += y;

      if(z < zmin){ zmin = z; }
      if(z > zmax){ zmax = z; }
      zsum += z;
    }

    fprintf(flog, "\n# Node stats\n");
    fprintf(flog, "X: min=%.10g max=%.10g avg=%.10g\n", xmin, xmax, xsum/(nodes.size?nodes.size:1));
    fprintf(flog, "Y: min=%.10g max=%.10g avg=%.10g\n", ymin, ymax, ysum/(nodes.size?nodes.size:1));
    fprintf(flog, "Z: min=%.10g max=%.10g avg=%.10g\n", zmin, zmax, zsum/(nodes.size?nodes.size:1));

    fprintf(flog, "\n# UNITSYS MN-MM=%s (E->Pa, F->N converted if YES)\n", unitsys_mnmm ? "YES" : "NO");
    fprintf(flog, "# Material name: %s\n", g_material_name[0] ? g_material_name : "(unknown)");

    fclose(flog);
  }

  fprintf(stdout, "Done: %s\n", path_mesh);
  return 0;
}
