//------------------------------------------------------------------------------
// GB_mex_band: C = tril (triu (A,lo), hi), or with A'
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2018, All Rights Reserved.
// http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

//------------------------------------------------------------------------------

// Apply a select operator to a matrix

#include "GB_mex.h"

#define FREE_ALL                        \
{                                       \
    GB_MATRIX_FREE (&C) ;               \
    GB_MATRIX_FREE (&A) ;               \
    GrB_free (&op) ;                    \
    GrB_free (&desc) ;                  \
    GB_mx_put_global (malloc_debug) ;   \
}

#define OK(method)                                      \
{                                                       \
    info = method ;                                     \
    if (info != GrB_SUCCESS)                            \
    {                                                   \
        FREE_ALL ;                                      \
        printf ("%s\n", GrB_error ()) ;                 \
        mexErrMsgTxt ("GraphBLAS failed") ;             \
    }                                                   \
}

bool band (const GrB_Index i, const GrB_Index j, const GrB_Index nrows,
    const GrB_Index ncols, const void *x, const void *k)
{
    int64_t *lohi = (int64_t *) k ;
    int64_t ii = (int64_t) i ;
    int64_t jj = (int64_t) j ;
//  printf ("i %lld j %lld lo %lld hi %lld\n", ii, jj, lohi [0], lohi [1]) ;
//  printf ("   j-i %lld\n", jj-ii) ;
    return ((lohi [0] <= (jj-ii)) && ((jj-ii) <= lohi [1])) ;
}

void mexFunction
(
    int nargout,
    mxArray *pargout [ ],
    int nargin,
    const mxArray *pargin [ ]
)
{

    bool malloc_debug = GB_mx_get_global ( ) ;
    GrB_Matrix C = NULL ;
    GrB_Matrix A = NULL ;
    GxB_SelectOp op = NULL ;
    GrB_Info info ;
    GrB_Descriptor desc = NULL ;

    #define GET_DEEP_COPY ;
    #define FREE_DEEP_COPY ;

    // check inputs
    if (nargout > 1 || nargin < 3 || nargin > 4)
    {
        mexErrMsgTxt ( "Usage: C = GB_mex_band (A, lo, hi, atranspose)") ;
    }

    // get A (shallow copy)
    A = GB_mx_mxArray_to_Matrix (pargin [0], "A input", false) ;
    if (A == NULL)
    {
        FREE_ALL ;
        mexErrMsgTxt ("A failed") ;
    }

    // get ho and hi
    int64_t lohi [2] ;
    lohi [0] = (int64_t) mxGetScalar (pargin [1]) ;
    lohi [1] = (int64_t) mxGetScalar (pargin [2]) ;

    // get atranspose
    bool atranspose = false ;
    if (nargin > 3) atranspose = (bool) mxGetScalar (pargin [3]) ;
    if (atranspose)
    {
        OK (GrB_Descriptor_new (&desc)) ;
        OK (GrB_Descriptor_set (desc, GrB_INP0, GrB_TRAN)) ;
    }

    // create operator
    METHOD (GxB_SelectOp_new (&op, band, NULL)) ;

    // create result matrix C
    if (atranspose)
    {
        OK (GrB_Matrix_new (&C, GrB_FP64, A->ncols, A->nrows)) ;
    }
    else
    {
        OK (GrB_Matrix_new (&C, GrB_FP64, A->nrows, A->ncols)) ;
    }

    // C<Mask> = accum(C,op(A))
    if (C->ncols == 1 && !atranspose)
    {
        // this is just to test the Vector version
        OK (GxB_select ((GrB_Vector) C, NULL, NULL, op, (GrB_Vector) A,
            lohi, NULL)) ;
    }
    else
    {
        OK (GxB_select (C, NULL, NULL, op, A, lohi, desc)) ;
    }

    // return C to MATLAB as a sparse matrix and free the GraphBLAS C
    pargout [0] = GB_mx_Matrix_to_mxArray (&C, "C output", false) ;

    FREE_ALL ;
}

