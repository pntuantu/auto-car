# Install script for directory: Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/auto-car")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "Z:/esp_tools/Espressif/tools/xtensa-esp-elf/esp-14.2.0_20241119/xtensa-esp-elf/bin/xtensa-esp32s3-elf-objdump.exe")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/mbedtls" TYPE FILE PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ FILES
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/aes.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/aria.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/asn1.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/asn1write.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/base64.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/bignum.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/block_cipher.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/build_info.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/camellia.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/ccm.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/chacha20.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/chachapoly.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/check_config.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/cipher.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/cmac.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/compat-2.x.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/config_adjust_legacy_crypto.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/config_adjust_legacy_from_psa.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/config_adjust_psa_from_legacy.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/config_adjust_psa_superset_legacy.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/config_adjust_ssl.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/config_adjust_x509.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/config_psa.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/constant_time.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/ctr_drbg.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/debug.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/des.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/dhm.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/ecdh.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/ecdsa.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/ecjpake.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/ecp.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/entropy.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/error.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/gcm.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/hkdf.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/hmac_drbg.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/lms.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/mbedtls_config.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/md.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/md5.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/memory_buffer_alloc.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/net_sockets.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/nist_kw.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/oid.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/pem.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/pk.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/pkcs12.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/pkcs5.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/pkcs7.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/platform.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/platform_time.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/platform_util.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/poly1305.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/private_access.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/psa_util.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/ripemd160.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/rsa.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/sha1.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/sha256.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/sha3.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/sha512.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/ssl.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/ssl_cache.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/ssl_ciphersuites.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/ssl_cookie.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/ssl_ticket.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/threading.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/timing.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/version.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/x509.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/x509_crl.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/x509_crt.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/x509_csr.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/psa" TYPE FILE PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ FILES
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/build_info.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_adjust_auto_enabled.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_adjust_config_dependencies.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_adjust_config_key_pair_types.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_adjust_config_synonyms.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_builtin_composites.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_builtin_key_derivation.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_builtin_primitives.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_compat.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_config.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_driver_common.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_driver_contexts_composites.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_driver_contexts_key_derivation.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_driver_contexts_primitives.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_extra.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_legacy.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_platform.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_se_driver.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_sizes.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_struct.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_types.h"
    "Z:/esp_tools/Espressif/frameworks/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_values.h"
    )
endif()

