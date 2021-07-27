// evmone: Fast Ethereum Virtual Machine implementation
// Copyright 2021 The evmone Authors.
// SPDX-License-Identifier: Apache-2.0

#include <evmone/eof.hpp>
#include <gtest/gtest.h>
#include <test/utils/utils.hpp>

using namespace evmone;

namespace
{
inline EOFValidationErrror validate_eof(bytes_view code, evmc_revision rev = EVMC_SHANGHAI) noexcept
{
    return ::validate_eof(rev, code.data(), code.size());
}
}  // namespace

TEST(eof_validation, validate_empty_code)
{
    EXPECT_EQ(validate_eof({}), EOFValidationErrror::invalid_prefix);
}

TEST(eof_validation, validate_EOF_prefix)
{
    EXPECT_EQ(validate_eof(from_hex("00")), EOFValidationErrror::invalid_prefix);
    EXPECT_EQ(validate_eof(from_hex("FE")), EOFValidationErrror::invalid_prefix);
    EXPECT_EQ(validate_eof(from_hex("EF")), EOFValidationErrror::invalid_prefix);

    EXPECT_EQ(validate_eof(from_hex("EFCA")), EOFValidationErrror::invalid_prefix);
    EXPECT_EQ(validate_eof(from_hex("EFCBFE01")), EOFValidationErrror::invalid_prefix);
    EXPECT_EQ(validate_eof(from_hex("EFCAFF01")), EOFValidationErrror::invalid_prefix);

    EXPECT_EQ(validate_eof(from_hex("EFCAFE")), EOFValidationErrror::eof_version_unknown);

    EXPECT_EQ(
        validate_eof(from_hex("EFCAFE01")), EOFValidationErrror::section_headers_not_terminated);
    EXPECT_EQ(
        validate_eof(from_hex("EFCAFE02")), EOFValidationErrror::section_headers_not_terminated);
}

// TODO tests from pre-Shanghai

TEST(eof_validation, validate_EOF_version)
{
    EXPECT_EQ(validate_eof(from_hex("EFCAFE03")), EOFValidationErrror::eof_version_unknown);
    EXPECT_EQ(validate_eof(from_hex("EFCAFEFF")), EOFValidationErrror::eof_version_unknown);
}

TEST(eof_validation, minimal_valid_EOF1_code)
{
    EXPECT_EQ(validate_eof(from_hex("EFCAFE01 010001 00 FE")), EOFValidationErrror::success);
}

TEST(eof_validation, minimal_valid_EOF1_code_with_data)
{
    EXPECT_EQ(
        validate_eof(from_hex("EFCAFE01 010001 020001 00 FE DA")), EOFValidationErrror::success);
}

TEST(eof_validation, EOF1_code_section_missing)
{
    EXPECT_EQ(validate_eof(from_hex("EFCAFE01 00")), EOFValidationErrror::code_section_missing);
    EXPECT_EQ(
        validate_eof(from_hex("EFCAFE01 020001 DA")), EOFValidationErrror::code_section_missing);
}

TEST(eof_validation, EOF1_code_section_0_size)
{
    EXPECT_EQ(validate_eof(from_hex("EFCAFE01 010000 020001 00 DA")),
        EOFValidationErrror::zero_section_size);
}

TEST(eof_validation, EOF1_data_section_0_size)
{
    EXPECT_EQ(validate_eof(from_hex("EFCAFE01 010001 020000 00 FE")),
        EOFValidationErrror::zero_section_size);
}

TEST(eof_validation, EOF1_multiple_code_sections)
{
    EXPECT_EQ(validate_eof(from_hex("EFCAFE01 010001 010001 00 FE FE")),
        EOFValidationErrror::multiple_code_sections);
    EXPECT_EQ(validate_eof(from_hex("EFCAFE01 010001 010001 020001 00 FE FE DA")),
        EOFValidationErrror::multiple_code_sections);
}

TEST(eof_validation, EOF1_multiple_data_sections)
{
    EXPECT_EQ(validate_eof(from_hex("EFCAFE01 010001 020001 020001 00 FE DA DA")),
        EOFValidationErrror::multiple_data_sections);
}

TEST(eof_validation, EOF1_table_section)
{
    EXPECT_EQ(validate_eof(from_hex("EFCAFE01 010001 030002 00 FE 0001")),
        EOFValidationErrror::unknown_section_id);

    EXPECT_EQ(validate_eof(from_hex("EFCAFE01 010001 020001 030002 00 FE DA 0001")),
        EOFValidationErrror::unknown_section_id);
}

TEST(eof_validation, EOF1_undefined_opcodes)
{
    // TODO replace with loop over evmone::instr::gas_costs

    EXPECT_EQ(validate_eof(from_hex("EFCAFE01 010001 00 0C")),
        EOFValidationErrror::undefined_instruction);
    EXPECT_EQ(validate_eof(from_hex("EFCAFE01 010001 00 21")),
        EOFValidationErrror::undefined_instruction);
    EXPECT_EQ(validate_eof(from_hex("EFCAFE01 010001 00 B0")),
        EOFValidationErrror::undefined_instruction);
    EXPECT_EQ(validate_eof(from_hex("EFCAFE01 010001 00 C0")),
        EOFValidationErrror::undefined_instruction);
    EXPECT_EQ(validate_eof(from_hex("EFCAFE01 010001 00 EF")),
        EOFValidationErrror::undefined_instruction);

    EXPECT_EQ(validate_eof(from_hex("EFCAFE01 010001 00 FE")), EOFValidationErrror::success);
}

TEST(eof_validation, EOF1_truncated_push)
{
    // TODO replace with loops

    EXPECT_EQ(validate_eof(from_hex("EFCAFE01 010001 00 60")), EOFValidationErrror::truncated_push);

    EXPECT_EQ(validate_eof(from_hex("EFCAFE01 010001 00 61")), EOFValidationErrror::truncated_push);
    EXPECT_EQ(
        validate_eof(from_hex("EFCAFE01 010002 00 6100")), EOFValidationErrror::truncated_push);

    EXPECT_EQ(validate_eof(from_hex("EFCAFE01 010001 00 62")), EOFValidationErrror::truncated_push);
    EXPECT_EQ(
        validate_eof(from_hex("EFCAFE01 010002 00 6200")), EOFValidationErrror::truncated_push);
    EXPECT_EQ(
        validate_eof(from_hex("EFCAFE01 010003 00 620000")), EOFValidationErrror::truncated_push);

    EXPECT_EQ(
        validate_eof(from_hex(
            "EFCAFE01 010020 00 7F00000000000000000000000000000000000000000000000000000000000000")),
        EOFValidationErrror::truncated_push);
}

TEST(eof_validation, EOF1_complete_push)
{
    // TODO replace with loop

    EXPECT_EQ(validate_eof(from_hex("EFCAFE01 010002 00 6000")), EOFValidationErrror::success);
    EXPECT_EQ(validate_eof(from_hex("EFCAFE01 010003 00 610000")), EOFValidationErrror::success);
    EXPECT_EQ(validate_eof(from_hex("EFCAFE01 010004 00 62000000")), EOFValidationErrror::success);
    EXPECT_EQ(
        validate_eof(from_hex("EFCAFE01 010005 00 6300000000")), EOFValidationErrror::success);
    EXPECT_EQ(validate_eof(
                  from_hex("EFCAFE01 010021 00 "
                           "7F0000000000000000000000000000000000000000000000000000000000000000")),
        EOFValidationErrror::success);
}

TEST(eof_validation, minimal_valid_EOF2)
{
    EXPECT_EQ(validate_eof(from_hex("EFCAFE02 010001 00 FE")), EOFValidationErrror::success);

    EXPECT_EQ(
        validate_eof(from_hex("EFCAFE02 010001 020001 00 FE DA")), EOFValidationErrror::success);

    EXPECT_EQ(
        validate_eof(from_hex("EFCAFE02 010001 030002 00 FE 0001")), EOFValidationErrror::success);

    EXPECT_EQ(validate_eof(from_hex("EFCAFE02 010001 020001 030002 00 FE DA 0001")),
        EOFValidationErrror::success);
}

TEST(eof_validation, multiple_table_sections)
{
    EXPECT_EQ(validate_eof(from_hex("EFCAFE02 010001 030002 030004 00 FE 0001 00010002")),
        EOFValidationErrror::success);

    EXPECT_EQ(validate_eof(from_hex("EFCAFE02 010001 020001 030002 030004 00 FE DA 0001 00010002")),
        EOFValidationErrror::success);
}

TEST(eof_validation, EOF2_table_section_0_size)
{
    EXPECT_EQ(validate_eof(from_hex("EFCAFE02 010001 030000 00 FE")),
        EOFValidationErrror::zero_section_size);

    EXPECT_EQ(validate_eof(from_hex("EFCAFE02 010001 030002 030000 00 FE 0000")),
        EOFValidationErrror::zero_section_size);
}

TEST(eof_validation, EOF2_table_section_odd_size)
{
    EXPECT_EQ(validate_eof(from_hex("EFCAFE02 010001 030003 00 FE 000000")),
        EOFValidationErrror::odd_table_section_size);

    EXPECT_EQ(validate_eof(from_hex("EFCAFE02 010001 030002 030003 00 FE 0000 000000")),
        EOFValidationErrror::odd_table_section_size);
}
// Test cases:
// Missing immediates
TEST(eof_validation, EOF2_rjump_truncated)
{
    EXPECT_EQ(validate_eof(from_hex("EFCAFE02 010001 00 5C")),
        EOFValidationErrror::missing_immediate_argument);

    EXPECT_EQ(validate_eof(from_hex("EFCAFE02 010002 00 5C00")),
        EOFValidationErrror::missing_immediate_argument);
}

TEST(eof_validation, EOF2_rjumpi_truncated)
{
    EXPECT_EQ(validate_eof(from_hex("EFCAFE02 010001 00 5D")),
        EOFValidationErrror::missing_immediate_argument);

    EXPECT_EQ(validate_eof(from_hex("EFCAFE02 010002 00 5D00")),
        EOFValidationErrror::missing_immediate_argument);
}

TEST(eof_validation, EOF2_rjumptable_truncated)
{
    EXPECT_EQ(validate_eof(from_hex("EFCAFE02 010001 00 5E")),
        EOFValidationErrror::missing_immediate_argument);

    EXPECT_EQ(validate_eof(from_hex("EFCAFE02 010002 00 5E00")),
        EOFValidationErrror::missing_immediate_argument);
}

// Jump* outside of code section
// Jump* into push data
// Jump* into jump immediate
// Invalid rjumptable index
// Rjumptable immediate destination
// Rjumptable out of bounds destination