/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2014 Intel Corporation. All rights reserved.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/queue.h>
#include <stdarg.h>

#include <rte_common.h>
#include <rte_log.h>
#include <rte_debug.h>
#include <rte_memory.h>
#include <rte_memzone.h>
#include <rte_ether.h>
#include <rte_malloc.h>
#include <rte_launch.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_atomic.h>
#include <rte_branch_prediction.h>
#include <rte_ring.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_string_fns.h>


#include "cpa.h"
#include "cpa_types.h"
#include "cpa_dc_dp.h"
#include "cpa_dc.h"

#define SAMPLE_MAX_BUFF 1024
#define TIMEOUT_MS 5000 /* 5 seconds */
#define SINGLE_INTER_BUFFLIST 1

/*
 * Callback function
 *
 * This function is "called back" (invoked by the implementation of
 * the API) when the asynchronous operation has completed.  The
 * context in which it is invoked depends on the implementation, but
 * as described in the API it should not sleep (since it may be called
 * in a context which does not permit sleeping, e.g. a Linux bottom
 * half).
 */
static void dcCallback(void *pCallbackTag, CpaStatus status)
{
    printf("Callback called with status = %d.\n", status);
}


/*
 * This function read the content of a file, request hardward
 * assist compression and write the compressed content to 
 * another file.
 *
 * Part of this function reference the sample code in the Intel 
 * QuickAssist software code in buffer management.
 *
 */
static CpaStatus compress_file(CpaInstanceHandle dcInstHandle, CpaDcSessionHandle sessionHdl)
{
    CpaStatus      status = CPA_STATUS_SUCCESS;
    Cpa8U         *pBufferMetaSrc = NULL;
    Cpa8U         *pBufferMetaDst = NULL;
    Cpa8U         *pBufferMetaDst2 = NULL;
    Cpa32U         bufferMetaSize = 0;
    CpaBufferList *pBufferListSrc = NULL;
    CpaBufferList *pBufferListDst = NULL;
    CpaBufferList *pBufferListDst2 = NULL;
    CpaFlatBuffer *pFlatBuffer = NULL;
    Cpa32U         bufferSize = sizeof(sampleData);
    Cpa32U         numBuffers = 1;  /* only using 1 buffer in this case */
    int            input_fd, output_fd;

    /* 
     * allocate memory for bufferlist and array of flat buffers in a contiguous
     * area and carve it up to reduce number of memory allocations required. 
     */
    Cpa32U bufferListMemSize = sizeof(CpaBufferList) + (numBuffers * sizeof(CpaFlatBuffer));
    Cpa8U *pSrcBuffer = NULL;
    Cpa8U *pDstBuffer = NULL;
    Cpa8U *pDst2Buffer = NULL;

    /* 
     * The following variables are allocated on the stack because we block
     * until the callback comes back. If a non-blocking approach was to be
     * used then these variables should be dynamically allocated 
     */
    CpaDcRqResults dcResults;
    struct COMPLETION_STRUCT complete;

    printf("cpaDcBufferListGetMetaSize\n");

    /*
     * Different implementations of the API require different
     * amounts of space to store meta-data associated with buffer
     * lists.  We query the API to find out how much space the current
     * implementation needs, and then allocate space for the buffer
     * meta data, the buffer list, and for the buffer itself.
     */
    status = cpaDcBufferListGetMetaSize( dcInstHandle, numBuffers, &bufferMetaSize);

    /* Allocte source buffer */
    if (CPA_STATUS_SUCCESS == status)
    {
        status = PHYS_CONTIG_ALLOC(&pBufferMetaSrc, bufferMetaSize);
    }
    if (CPA_STATUS_SUCCESS == status)
    {
        status = OS_MALLOC(&pBufferListSrc, bufferListMemSize);
    }
    if (CPA_STATUS_SUCCESS == status)
    {
        status = PHYS_CONTIG_ALLOC(&pSrcBuffer, bufferSize);
    }

    /* Allocte destination buffer the same size as source buffer */
    if (CPA_STATUS_SUCCESS == status)
    {
        status = PHYS_CONTIG_ALLOC(&pBufferMetaDst, bufferMetaSize);
    }
    if (CPA_STATUS_SUCCESS == status)
    {
        status = OS_MALLOC(&pBufferListDst, bufferListMemSize);
    }
    if (CPA_STATUS_SUCCESS == status)
    {
        status = PHYS_CONTIG_ALLOC(&pDstBuffer, bufferSize);
    }

    if (CPA_STATUS_SUCCESS == status)
    {

        /* Open the input file - "uncompressFile" in the same directory */
        input_fd = open("uncompressFile", O_RDONLY);
        if ( input_fd == -1 )
        {
            printf( "Unable to open input file. Nothing to compress.\n");
        }
        fseek(input_fd, 0, SEEK_END);
        file_size - ftell(input_fd);
        fseek(input_fd, 0, SEEK_SET);
        if ( file_size > bufferListMemSize )
        {
            printf( "File size is too big.\n");
            printf( "Not able to read in all data into buffer allocated\n");
            close (input_fd);
        }
        
        /* copy content of the input file into the source buffer */
        fread(pSrcBuffer, file_size, input_file);

        /* Build source bufferList */
        pFlatBuffer = (CpaFlatBuffer *) (pBufferListSrc + 1);

        pBufferListSrc->pBuffers         = pFlatBuffer;
        pBufferListSrc->numBuffers       = 1;
        pBufferListSrc->pPrivateMetaData = pBufferMetaSrc;

        pFlatBuffer->dataLenInBytes = bufferSize;
        pFlatBuffer->pData          = pSrcBuffer;

        /* Build destination bufferList */
        pFlatBuffer = (CpaFlatBuffer *) (pBufferListDst + 1);

        pBufferListDst->pBuffers         = pFlatBuffer;
        pBufferListDst->numBuffers       = 1;
        pBufferListDst->pPrivateMetaData = pBufferMetaDst;

        pFlatBuffer->dataLenInBytes = bufferSize;
        pFlatBuffer->pData          = pDstBuffer;

       /*
        * Now, we initialize the completion variable which is used by the callback
        * function to indicate that the operation is complete.  We then perform the
        * operation.
        */
        printf("cpaDcCompressData\n");

        COMPLETION_INIT(&complete);

        status = cpaDcCompressData(dcInstHandle,
                                   sessionHdl,
                                   pBufferListSrc,     /* source buffer list */
                                   pBufferListDst,     /* destination buffer list */
                                   &dcResults,         /* results structure */
                                   CPA_DC_FLUSH_FINAL, /* Stateless session */
                                   (void *)&complete); /* data sent as is to the callback function*/

        if (CPA_STATUS_SUCCESS != status)
        {
            printf("cpaDcCompressData failed. (status = %d)\n", status);
        }

        /*
         * We now wait until the completion of the operation.  This uses a macro
         * which can be defined differently for different OSes.
         */
        if (CPA_STATUS_SUCCESS == status)
        {
            if (!COMPLETION_WAIT(&complete, TIMEOUT_MS))
            {
                printf("timeout or interruption in cpaDcCompressData\n");
                status = CPA_STATUS_FAIL;
            }
        }

        /*
         * We now check the results
         */
        if (CPA_STATUS_SUCCESS == status)
        {
            if (dcResults.status != CPA_DC_OK)
            {
               printf("Results status not as expected (status = %d)\n", dcResults.status);
               status = CPA_STATUS_FAIL;
            }
            else
            {
                 printf("Data consumed %d\n", dcResults.consumed);
                 printf("Data produced %d\n", dcResults.produced);
                 printf("Adler checksum 0x%x\n", dcResults.checksum);

                 output_fd = open("compressFile", O_RDWR);
                 if ( input_fd == -1 )
                 {
                     printf( "Unable to open output file. Nothing to verify.\n");
                 }
                 fwrite(pBufferListDst->pBuffers, 1, dcResults.produced, output_fd);
            }
        }

    }

    /*
     * At this stage, the callback function has returned, so it is
     * sure that the structures won't be needed any more.  Free the
     * memory!
     */
    PHYS_CONTIG_FREE(pSrcBuffer);
    OS_FREE(pBufferListSrc);
    PHYS_CONTIG_FREE(pBufferMetaSrc);
    PHYS_CONTIG_FREE(pDstBuffer);
    OS_FREE(pBufferListDst);
    PHYS_CONTIG_FREE(pBufferMetaDst);
    PHYS_CONTIG_FREE(pDst2Buffer);
    OS_FREE(pBufferListDst2);
    PHYS_CONTIG_FREE(pBufferMetaDst2);

    COMPLETION_DESTROY(&complete);

    close (input_fd);
    close (output_fd);
    return status;
}

#define MAX_INSTANCES 1

/*
 * 
 * NOTE: this code is work in progress and is not tested. 
 * 
 * Main function for the Proof Of Concept code.  The flow and initialization
 * logic is taken from the sample code provied in the Intel QuickAssist 
 * tree.
 *
 * Basically in this function we need to
 *
 *   1. Initialize an instance
 *   2. Initialize a session
 *   3. Start a session
 *   4. Perform the hardware assisted related operations
 *   5. Remove the session
 *   6. Program return
 *
 * To Do:
 *  1. I still need to resvole the MARCROs for memory management as they are not in
 *     the DPDK context but in the Intel QAT code base.
 *  2. Resource clean up will also need to be look at on error conditions.
 *
 */
void poc_compress(void)
{
    CpaStatus                  status = CPA_STATUS_SUCCESS;
    CpaDcInstanceCapabilities  cap = {0};
    CpaBufferList             **bufferInterArray = NULL;
    Cpa16U                     numInterBuffLists = 0;
    Cpa16U                     bufferNum = 0;
    Cpa32U                     buffMetaSize = 0;

    Cpa32U                sess_size = 0;
    Cpa32U                ctx_size = 0;
    CpaDcSessionHandle    sessionHdl = NULL;
    CpaInstanceHandle     dcInstHandle = NULL;
    CpaDcSessionSetupData sd = {0};
    CpaDcStats            dcStats = {0};

    CpaInstanceHandle dcInstHandles[MAX_INSTANCES];
    Cpa16U            numInstances = 0;

    /*
     * This is a Proof Of Concept only, taking the first 
     * instance of a data compression service.
     */
    status = cpaDcGetNumInstances(&numInstances);
    if ((status == CPA_STATUS_SUCCESS) && (numInstances > 0))
    {
        status = cpaDcGetInstances(MAX_INSTANCES, dcInstHandles);
        if (status == CPA_STATUS_SUCCESS)
        {
            *pDcInstHandle = dcInstHandles[0];
        }
        else
        {   
            printf( "Failed to obtain Instance handle \n");
            printf( "Programing terminating ...\n);
            return;
        }
    }
    else 
    {
        printf("Unable to find an instance to perform the compression operations\n");
        printf("Program terminating ...\n");
        return;
    }

    if (dcInstHandle == NULL)
    {
        printf("Unable to allocate an instance to perform the compression operations\n");
        printf("Program terminating ...\n");
        return;
    }

    /* Query Capabilities */
    printf("cpaDcQueryCapabilities\n");
    Cpastatus = cpaDcQueryCapabilities(dcInstHandle, &cap);
    if ( status != CPA_STATUS_SUCCESS )
    {
        printf("Unable to determine the capability of the hardware\n");
        printf("Program terminating ...\n");
        cpaDcStopInstance(dcInstHandle);
        return;
    }

    if ( !cap.statelessDeflateCompression ||
         !cap.statelessDeflateDecompression ||
         !cap.checksumAdler32 ||
         !cap.dynamicHuffman )
    {
        printf("Error: Unsupported functionality\n");
        printf("Program terminating ...\n");
        cpaDcStopInstance(dcInstHandle);
        return;
    }

    if ( cap.dynamicHuffmanBufferReq )
    {
        status = cpaDcBufferListGetMetaSize(dcInstHandle, 1, &buffMetaSize);

        if (CPA_STATUS_SUCCESS == status)
        {
            status = cpaDcGetNumIntermediateBuffers(dcInstHandle, &numInterBuffLists);
        }

        if (CPA_STATUS_SUCCESS == status && 0 != numInterBuffLists)
        {
            status = PHYS_CONTIG_ALLOC(&bufferInterArray, numInterBuffLists*sizeof(CpaBufferList *));
        }

        for (bufferNum = 0; bufferNum < numInterBuffLists; bufferNum++)
        {
            if (CPA_STATUS_SUCCESS == status)
            {
                status = PHYS_CONTIG_ALLOC( &bufferInterArray[bufferNum],sizeof(CpaBufferList));
            }

            if (CPA_STATUS_SUCCESS == status)
            {
                status = PHYS_CONTIG_ALLOC( &bufferInterArray[bufferNum]->pPrivateMetaData, buffMetaSize);
            }

            if (CPA_STATUS_SUCCESS == status)
            {
                status = PHYS_CONTIG_ALLOC( &bufferInterArray[bufferNum]->pBuffers, sizeof(CpaFlatBuffer));
            }

            if (CPA_STATUS_SUCCESS == status)
            {
                /* 
                 * Implementation requires an intermediate buffer approximately
                 * twice the size of the output buffer 
                 */
                status = PHYS_CONTIG_ALLOC( &bufferInterArray[bufferNum]->pBuffers->pData, 2*SAMPLE_MAX_BUFF);
                bufferInterArray[bufferNum]->numBuffers               = 1;
                bufferInterArray[bufferNum]->pBuffers->dataLenInBytes = 2*SAMPLE_MAX_BUFF;
            }

        } /* End numInterBuffLists */
    }

    if (CPA_STATUS_SUCCESS == status)
    {
       /* Start DataCompression component */
       printf("cpaDcStartInstance\n");
       status = cpaDcStartInstance(dcInstHandle, 
                                   numInterBuffLists, 
                                   bufferInterArray);
        if ( status != CPA_STATUS_SUCCESS )
        {
            printf( "Fail to start an instance\n");
            printf( "Program terminating ...\n");
            cpaDcStopInstance(dcInstHandle);
            return;
        }
    }
    else
    {
        printf( "Fail to allocate buffer for dynamic Hufferman tree\n");
        printf( "Program terminating ...\n");
        cpaDcStopInstance(dcInstHandle);
        return;
    }

    /*
     * We now populate the fields of the session operational data and create
     * the session.  Note that the size required to store a session is
     * implementation-dependent, so we query the API first to determine how
     * much memory to allocate, and then allocate that memory.
     */
     sd.compLevel = CPA_DC_L4;
     sd.compType  = CPA_DC_DEFLATE;
     sd.huffType  = CPA_DC_HT_FULL_DYNAMIC;
     /* 
      * If the implementation supports it, the session will be configured
      * to select static Huffman encoding over dynamic Huffman as
      * the static encoding will provide better compressibility.
      */
     if (cap.autoSelectBestHuffmanTree)
     {
        sd.autoSelectBestHuffmanTree = CPA_TRUE;
     }
     else
     {
        sd.autoSelectBestHuffmanTree = CPA_FALSE;
     }

     sd.sessDirection = CPA_DC_DIR_COMBINED;
     sd.sessState     = CPA_DC_STATELESS;

#if ( CPA_DC_API_VERSION_NUM_MAJOR == 1 && CPA_DC_API_VERSION_NUM_MINOR < 6 )
     sd.deflateWindowSize = 7;
#endif
     sd.checksum = CPA_DC_ADLER32;

     /* Determine size of session context to allocate */
     printf("cpaDcGetSessionSize\n");
     status = cpaDcGetSessionSize(dcInstHandle,
                                     &sd, 
                                     &sess_size, 
                                     &ctx_size);

    if (CPA_STATUS_SUCCESS == status)
    {
        /* Allocate session memory */
        status = PHYS_CONTIG_ALLOC(&sessionHdl, sess_size);
        if ( status != CPA_STATUS_SUCCESS )
        {
            printf( "Faile to allocate memory for session handle\n");
            printf( "Program terminating ...\n");
            cpaDcStopInstance(dcInstHandle);
            PHYS_CONTIG_FREE(sessionHdl);
            return;
        }
    }
    else
    {
        printf("Unable to get session size\n");
        printf("Program terminating ...\n");
        cpaDcStopInstance(dcInstHandle);
        return;
    }

    /* Initialize the Stateless session */
    printf("cpaDcInitSession\n");
    status = cpaDcInitSession(dcInstHandle,
                              sessionHdl,  /* session memory */
                              &sd,         /* session setup data */
                              NULL,        /* pContexBuffer not required for stateless operations */
                              dcCallback); /* callback function */

    if (CPA_STATUS_SUCCESS == status)
    {
        CpaStatus sessionStatus = CPA_STATUS_SUCCESS;

        /* 
         *Perform Compression operation by calling the QuickAssist API. 
         */
        status = compress_file(dcInstHandle, sessionHdl);

        if ( status != CPA_STATUS_SUCCESS )
        {
            printf( "Compression operation by hardward failed.\n");
        `   printf( "Program terminating ...\n);
            cpaDcStopInstance(dcInstHandle);
            PHYS_CONTIG_FREE(sessionHdl);
            return;
        }

        /*
         * In a typical usage, the session might be used to compression
         * multiple buffers.  In this example however, we can now
         * tear down the session.
         */
        printf("cpaDcRemoveSession\n");
        sessionStatus = cpaDcRemoveSession( dcInstHandle, sessionHdl);

        /* Maintain status of remove session only when status of all operations
         * before it are successful. */
        if (CPA_STATUS_SUCCESS == status)
        {
            status = sessionStatus;
        }
    }
    else
    {
        printf("Unable to intialize session\n");
        printf("Program terminating ...\n");
        cpaDcStopInstance(dcInstHandle);
        PHYS_CONTIG_FREE(sessionHdl);
        return;
    }

    if (CPA_STATUS_SUCCESS == status)
    {
        /*
         * We can now query the statistics on the instance.
         *
         * Note that some implementations may also make the stats
         * available through other mechanisms, e.g. in the /proc
         * virtual filesystem.
         */
        status = cpaDcGetStats(dcInstHandle, &dcStats);

        if (CPA_STATUS_SUCCESS != status)
        {
            printf("cpaDcGetStats failed, status = %d\n", status);
        }
        else
        {
            printf("Number of compression operations completed: %llu\n",
                        (unsigned long long)dcStats.numCompCompleted);
            printf("Number of decompression operations completed: %llu\n",
                    (unsigned long long)dcStats.numDecompCompleted);
        }
    }

    /*
     * Cleaning up: Free up memory, stop the instance, etc.
     */

    printf("cpaDcStopInstance\n");
    cpaDcStopInstance(dcInstHandle);

    /* Free session Context */
    PHYS_CONTIG_FREE(sessionHdl);

    /* Free intermediate buffers */
    if (bufferInterArray != NULL)
    {
        for (bufferNum = 0; bufferNum < numInterBuffLists; bufferNum++)
        {
            PHYS_CONTIG_FREE(bufferInterArray[bufferNum]->pBuffers->pData);
            PHYS_CONTIG_FREE(bufferInterArray[bufferNum]->pBuffers);
            PHYS_CONTIG_FREE(bufferInterArray[bufferNum]->pPrivateMetaData);
            PHYS_CONTIG_FREE(bufferInterArray[bufferNum]);
        }
        PHYS_CONTIG_FREE(bufferInterArray);
    }

    if (CPA_STATUS_SUCCESS == status)
    {
        printf("Proof of Concept code for compression ran successfully\n");
    }
    else
    {
        printf("Proof of Concept code for compression failed with status of %d\n", status);
    }

    return;
}
