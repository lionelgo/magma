#!/bin/bash

set -euo pipefail

CONFIG_DIR=${PREFIX}/etc

pushd ${PREFIX}/scripts
./check_mme_s6a_certificate ${CONFIG_DIR} ${MME_FQDN}
popd

exec "$@"
