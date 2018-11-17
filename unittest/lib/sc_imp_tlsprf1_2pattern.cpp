//
// Pattern file for the Symcrypt TLS PRF 1.2 implementations.
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//

template<>
VOID
KdfImp<ImpSc, AlgTlsPrf1_2, BaseAlgXxx>::derive(
    _In_reads_(cbKey)       PCBYTE          pbKey,
                            SIZE_T          cbKey,
    _In_                    PKDF_ARGUMENTS  pArgs,
    _Out_writes_(cbDst)     PBYTE           pbDst,
                            SIZE_T          cbDst)
{
    BYTE buf1[1024];
    BYTE buf2[sizeof(buf1)];
    SYMCRYPT_ERROR scError;
    SYMCRYPT_TLSPRF1_2_EXPANDED_KEY expandedKey;
    BYTE expandedKeyChecksum[SYMCRYPT_MARVIN32_RESULT_SIZE];

    PCBYTE  pbLabel;
    SIZE_T  cbLabel;
    PCBYTE  pbSeed;
    SIZE_T  cbSeed;

    CHECK(cbDst <= sizeof(buf1), "TLS PRF 1.2 output too large");

    switch (pArgs->argType)
    {
        case KdfArgumentTlsPrf:
            pbLabel = pArgs->uTlsPrf.pbLabel;
            cbLabel = pArgs->uTlsPrf.cbLabel;
            pbSeed = pArgs->uTlsPrf.pbSeed;
            cbSeed = pArgs->uTlsPrf.cbSeed;
            break;

        default:
            CHECK(FALSE, "Unknown argument type for TLS PRF 1.2");
            return;
    }

    initXmmRegisters();
    scError = SymCryptTlsPrf1_2(
        SYMCRYPT_BaseXxxAlgorithm,
        pbKey, cbKey,
        pbLabel, cbLabel,
        pbSeed, cbSeed,
        &buf1[0], cbDst);
    verifyXmmRegisters();

    CHECK(scError == SYMCRYPT_NO_ERROR, "Error in SymCrypt TLS PRF 1.2");

    scError = SymCryptTlsPrf1_2ExpandKey( &expandedKey,
                                          SYMCRYPT_BaseXxxAlgorithm,
                                          pbKey, cbKey);
    verifyXmmRegisters();
    CHECK(scError == SYMCRYPT_NO_ERROR, "Error in SymCrypt TLS PRF 1.2");

    SymCryptMarvin32(SymCryptMarvin32DefaultSeed, (PCBYTE)&expandedKey, sizeof(expandedKey), expandedKeyChecksum);

    scError = SymCryptTlsPrf1_2Derive( &expandedKey,
                                       pbLabel, cbLabel,
                                       pbSeed, cbSeed,
                                       &buf2[0], cbDst);
    verifyXmmRegisters();
    CHECK(scError == SYMCRYPT_NO_ERROR, "Error in SymCrypt TLS PRF 1.2");

    CHECK(memcmp(buf1, buf2, cbDst) == 0, "SymCrypt TLS PRF 1.2 calling versions disagree");

    SymCryptMarvin32(SymCryptMarvin32DefaultSeed, (PCBYTE)&expandedKey, sizeof(expandedKey), buf2);
    CHECK(memcmp(expandedKeyChecksum, buf2, SYMCRYPT_MARVIN32_RESULT_SIZE) == 0, "SymCrypt TLS PRF 1.2 modified expanded key");

    memcpy(pbDst, buf1, cbDst);

}

template<>
VOID
algImpDataPerfFunction<ImpXxx, AlgXxx, BaseAlgXxx>(PBYTE buf1, PBYTE buf2, PBYTE buf3, SIZE_T dataSize)
{
    SymCryptTlsPrf1_2Derive((PCSYMCRYPT_TLSPRF1_2_EXPANDED_KEY)buf1, NULL, 0, buf2, 32, buf3, dataSize);
}

