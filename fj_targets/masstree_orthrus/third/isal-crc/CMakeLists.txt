add_library(isal-crc
    crc_base.c
    crc64_base.c

    crc_base_aliases.c

    crc_ref.h
    crc64_ref.h

	crc16_t10dif_01.asm
	crc16_t10dif_by4.asm
	crc16_t10dif_02.asm
	crc16_t10dif_by16_10.asm
	crc16_t10dif_copy_by4.asm
	crc16_t10dif_copy_by4_02.asm
	crc32_ieee_01.asm
	crc32_ieee_02.asm
	crc32_ieee_by4.asm
	crc32_ieee_by16_10.asm
	crc32_iscsi_01.asm
	crc32_iscsi_00.asm
	crc32_iscsi_by16_10.asm
	crc_multibinary.asm
	crc64_multibinary.asm
	crc64_ecma_refl_by8.asm
	crc64_ecma_refl_by16_10.asm
	crc64_ecma_norm_by8.asm
	crc64_ecma_norm_by16_10.asm
	crc64_iso_refl_by8.asm
	crc64_iso_refl_by16_10.asm
	crc64_iso_norm_by8.asm
	crc64_iso_norm_by16_10.asm
	crc64_jones_refl_by8.asm
	crc64_jones_refl_by16_10.asm
	crc64_jones_norm_by8.asm
	crc64_jones_norm_by16_10.asm
	crc64_rocksoft_refl_by8.asm
	crc64_rocksoft_refl_by16_10.asm
	crc64_rocksoft_norm_by8.asm
	crc64_rocksoft_norm_by16_10.asm
	crc32_gzip_refl_by8.asm
	crc32_gzip_refl_by8_02.asm
	crc32_gzip_refl_by16_10.asm
)
target_include_directories(isal-crc PUBLIC include)
