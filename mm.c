/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "2",
    /* First member's full name */
    "Son YJ",

    /* First member's email address */
    "soja0529@gmail.com",
    
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

//--------------------------------------------------------
/* Basic constants and macros */
#define WSIZE 4             /* Word and header/footer sizxe (bytes) */
// 워드 사이즈

#define DSIZE 8             /* Double word size (bytes) */
// 더블워드 사이즈

#define CHUNKSIZE (1 << 12) /* Extend heap by this amount (bytes) */
// 힙 확장의 기본 크기(초기 빈 블록의 크기)

// 매크로
#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))
// 사이즈와 할당비트를 결합해서 header와 footer에 저장할 값

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
// p가 참조하는 워드 반환 (포인터라서 직접 역참조 불가능 -> 타입 캐스팅)

#define PUT(p, val) (*(unsigned int *)(p) = (val))
// p에 val 저장

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
// 사이즈를 만들 때 뒤에 세자리 날려 보내기

#define GET_ALLOC(p) (GET(p) & 0x1)
// 할당 시키기

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp)-WSIZE)
// 헤더 포인터

#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
// 푸터 포인터

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE)))
// 다음 블록의 포인터

#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))
// 이전 블록의 포인터
//--------------------------------------------------------
static void *extend_heap(size_t);
static void *coalesce(void *);
static void *find_fit(size_t);
static void place(void *, size_t);
static void *heap_listp;

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    // 힙 생성하기
    // 힙 초기 생성 : '정렬패딩+프롤로그 헤더+프롤로그 푸터+에필로그 헤더'를 위한 4워드 크기의 힙 생성
    // 생성 이후 청크사이즈 만큼 추가 메모리를 요청해서 힙의 크기를 확장
    /* Create the inital empty heap */
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0);                            /* Alignment padding */
    // 정렬패딩
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */
    // 프롤로그 헤더
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
    // 프롤로그 푸터
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     /* Epilogue header */
    // 에필로그 헤더 : 프로그램이 할당한 마지막 블록의 뒤에 위치하며, 블록이 할당되지 않은 상태를 나타냄
    heap_listp += (2 * WSIZE);

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    // 힙을 CHUNKSIZE bytes로 확장
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    // 가용 블록에 할당하기
    // 사이즈 조정 : 할당 요청이 들어오면(malloc 호출), 요청 받은 size를 조정
    // 블록의 크기는 최소 16바이트(헤더4 + 푸터4 + 페이로드8)

    // 가용 블록 검색
    // 조정된 사이즈를 가지고 있는 블록을 검색(find_fit 함수 호출)
    // 적합한 블록을 찾았으면 해당 블록에 할당(place 함수 호출)

    // 힙 확장
    // 적합한 블록이 없으면, 추가 메모리를 요청해서 힙을 확장(extend_heap 함수 호출)
    // 힙을 확장하고 나서 받은 새 메모리 블록에 할당(place 함수 호출)

    size_t asize;      /* Adjusted block size */
    // 조정된 블록 사이즈

    size_t extendsize; /* Amount to extend heap if no fit */
    // 확장할 사이즈

    char *bp;

    /* Ignore spurious requests */
    // 잘못된 요청 분기
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    /* 사이즈 조정 */

    if (size <= DSIZE)
    // 8바이트 이하이면
        asize = 2 * DSIZE;
        // 최소 블록 크기 16바이트 할당 (헤더 4 + 푸터 4 + 저장공간 8)
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
        // 8배수로 올림 처리

    /* Search the free list for a fit */
    /* 가용 블록 검색 */
    if ((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize);
        // 할당
        return bp;
        // 새로 할당된 블록의 포인터 리턴
    }

    /* No fit found. Get more memory and place the block */
    /* 적합한 블록이 없을 경우 힙확장 */
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    // 블록 반환하기
    // 가용 상태로 변경 : 블록의 상태를 할당 상태에서 가용 상태로 변경한다. (PACK 매크로 호출)
    // 병합 : 주소상으로 인접한 블록에 빈 블록이 있다면 연결한다. (coalesce 함수 호출)

    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */

// 기존에 할당된 메모리 블록의 크기 변경
// `기존 메모리 블록의 포인터`, `새로운 크기`
void *mm_realloc(void *ptr, size_t size)
{
    // 재할당하기

    // 예외 처리
    // 인자로 전달 받은 ptr이 NULL인 경우에는 할당만 수행한다. (mm_malloc 함수 호출)
    // 인자로 전달 받은 size가 0인 경우에는 메모리 반환만 수행한다. (mm_free 함수 호출)

    // 새 블록에 할당
    // 할당 : 인자로 전달 받은 size만큼 할당할 수 있는 블록을 찾아 할당한다. (mm_malloc 함수 호출)

    // 데이터 복사
    // ptr이 가리키고 있던 블록이 가지고 있는 payload 크기를 구하고,
    // 새로 할당할 size보다 기존의 크기가 크다면 size로 크기를 조정한다.
    // 조정한 size만큼 새로 할당한 블록으로 데이터를 복사한다. (memcpy 함수 호출)

    // 기존 블록 반환
    // 데이터 복사 후, 기존 블록의 메모리를 반환한다. (mm_free 함수 호출)

    if (size <= 0)
    {
        // size가 0인 경우 메모리 반환만 수행
        mm_free(ptr);
        return 0;
    }
    if (ptr == NULL)
    {
        /* 예외 처리 */
        return mm_malloc(size);
        // 포인터가 NULL인 경우 할당만 수행
    }

    /* 새 블록에 할당 */
    void *newp = mm_malloc(size);
    // 새로 할당한 블록의 포인터
    if (newp == NULL)
    {
        return 0;
        // 할당 실패
    }

    /* 데이터 복사 */
    size_t oldsize = GET_SIZE(HDRP(ptr));

    if (size < oldsize)
    // 기존 사이즈가 새 크기보다 더 크면
    {
        oldsize = size;
        // size로 크기 변경 (기존 메모리 블록보다 작은 크기에 할당하면, 일부 데이터만 복사)
    }
    memcpy(newp, ptr, oldsize);
    // 새 블록으로 데이터 복사

    /* 기존 블록 반환 */
    mm_free(ptr);

    return newp;
}

//------------------------------------------------------------
static void *extend_heap(size_t words)
{
    // 힙 확장
    // 추가 메모리 요청: 요청받은 words만큼 추가 메모리를 요청한다. (mem_sbrk 함수 요청)
    // 추가 메모리 요청에 성공하면 새로운 메모리를 새 빈 블록으로 만든다.
    // 새 빈 블록의 header, footer를 초기화하고 힙의 끝부분을 나타내는 에필로그 블록의 헤더도 변경한다.

    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    // 더블 워드 정렬 유지
    // size: 확장할 크기
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    // 2워드의 가장 가까운 배수로 반올림 (홀수면 1 더해서 곱함)

    if ((long)(bp = mem_sbrk(size)) == -1)
    // 힙 확장
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));         /* Free block header */
    // 새 빈 블록 헤더 초기화

    PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */
    // 새 빈 블록 푸터 초기화

    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */
    // 에필로그 블록 헤더 초기화

    /* Coalese if the previous block was free */
    return coalesce(bp);
    // 병합 후 coalesce 함수에서 리턴된 블록 포인터 반환
}

static void *coalesce(void *bp)
{
    // 병합
    // 할당 여부 확인 : 인접 블록의 할당 상태를 확인한다. (GET_ALLOC 매크로 호출)
    // 병합 : 인접 블록 중 가용 상태인 블록이 있다면 현재 블록과 합쳐서 더 큰 하나의 가용 블록으로 만든다.

    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    // 이전 블록 할당 상태

    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    // 다음 블록 할당 상태

    size_t size = GET_SIZE(HDRP(bp));
    // 현재 블록 사이즈

    if (prev_alloc && next_alloc)
    { /* Case 1 : 모두 할당된 경우 */
        return bp;
    }

    else if (prev_alloc && !next_alloc)
    { /* Case 2 : 다음 블록만 빈 경우 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        // 현재 블록 헤더 재설정
        PUT(FTRP(bp), PACK(size, 0));
        // 다음 블록 푸터 재설정 (위에서 헤더를 재설정했으므로, FTRP(bp)는 합쳐질 다음 블록의 푸터가 됨)
    }

    else if (!prev_alloc && next_alloc)
    { /* Case 3 : 이전 블록만 빈 경우 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        // 이전 블록 헤더 재설정
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        // 현재 블록 푸터 재설정
        bp = PREV_BLKP(bp);
        // 이전 블록의 시작점으로 포인터 변경
    }

    else
    { /* Case 4 : 이전 블록과 다음 블록 모두 빈 경우 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        // 이전 블록 헤더 재설정
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        // 다음 블록 푸터 재설정
        bp = PREV_BLKP(bp);
        // 이전 블록의 시작점으로 포인터 변경
    }
    return bp;
    // 병합된 블록의 포인터 반환
}

static void *find_fit(size_t asize)
{
    // 가용 블록 검색
    // 가용 상태이면서 현재 필요한 크기인 asize 를 담을 수 있는 블록을 찾을 때까지 탐색을 이어간다. (First-fit)

    void *bp;
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    // heap_listp : 첫번째 블록(주소: 힙의 첫 부분 + 8bytes)부터 탐색 시작
    // NEXT_BLKP : 조건에 맞지 않으면 다음 블록으로 이동해서 탐색을 이어감
    {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
        // 가용 상태이고, 사이즈가 적합하면
        {
            return bp;
            // 해당 블록 포인터 리턴
        }
    }
    return NULL;
}

static void place(void *bp, size_t asize)
{
    // 할당
    // 선택한 가용 블록이 사용할 사이즈보다 큰 사이즈를 가지고 있다면, 
    // 필요한 사이즈만큼만 사용하고 나머지는 분할하여 또다른 가용 블록으로 남겨둔다.

    size_t csize = GET_SIZE(HDRP(bp));
    // 현재 블록의 크기

    if ((csize - asize) >= (2 * DSIZE))
    // 차이가 최소 블록 크기 16보다 같거나 크면 분할
    {
        // 현재 블록에는 필요한 만큼만 할당
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));

        bp = NEXT_BLKP(bp);

        // 남은 크기를 다음 블록에 할당(가용 블록)
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
    }
    else
    {
        // 해당 블록 전부 사용
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}