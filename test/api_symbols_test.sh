#!/bin/sh
#
# telebot
#
# Copyright (c) 2026 Elmurod Talipov.
#
# Licensed under the Apache License, Version 2.0 (the License);
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Fails if a function declared in an installed public header is not defined
# in the library (a "declared but never implemented" API).
#
# Usage: api_symbols_test.sh <library> <include dir>

set -eu
lib=$1
inc=$2

# Exported, defined symbols; Mach-O prefixes C symbols with '_'
defined=$(nm -g "$lib" | awk '$2 ~ /^[TDBS]$/ {print $3}' | sed 's/^_//')

# Installed headers only: telebot-private.h and telebot-parser.h are internal
headers=$(ls "$inc"/telebot*.h | grep -v -e telebot-private.h -e telebot-parser.h)

declared=$(cat $headers |
    grep -E '^(telebot_error_e|telebot_core_response_t|const char \*|void|bool) \**telebot_[a-z0-9_]+\(' |
    sed -E 's/^[^(]*[ *](telebot_[a-z0-9_]+)\(.*/\1/' | sort -u)

missing=0
for fn in $declared; do
    if ! printf '%s\n' "$defined" | grep -qx "$fn"; then
        echo "declared but not defined: $fn"
        missing=$((missing + 1))
    fi
done

count=$(printf '%s\n' "$declared" | grep -c .)
echo "$count public functions checked, $missing missing"
[ "$missing" -eq 0 ]
