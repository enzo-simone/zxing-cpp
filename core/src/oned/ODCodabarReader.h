#pragma once
/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "ODRowReader.h"

namespace ZXing {

class DecodeHints;

namespace OneD {

/**
* <p>Decodes Codabar barcodes.</p>
*
* @author Bas Vijfwinkel
* @author David Walker
*/
class CodabarReader : public RowReader
{
public:
	explicit CodabarReader(const DecodeHints& hints);
	Result decodeRow(int rowNumber, const BitArray& row, std::unique_ptr<DecodingState>& state) const override;

private:
	bool _shouldReturnStartEnd;
};

} // OneD
} // ZXing
