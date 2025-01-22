#include <array>

#include <concat.hpp>

#define CONCAT_LINE_STRING_CONSTRUCT(head, start, before, between, end, endAndStartWith) {{{{head, start, before, end, endAndStartWith}}}}

const std::array<const CConcatLineString, 8> g_arrEmbedsConcat = 
{
	{
		// 0-3
		CONCAT_LINE_STRING_CONSTRUCT(
			"",     // Head with.
			"\t",   // Start with.
			":",    // Before value.
			": ",   // Between key & value.
			"\n",   // End.
			"\n\t"  // End and next line.
		),
		CONCAT_LINE_STRING_CONSTRUCT(
			"\t",
			"\t\t",
			":",
			": ",
			"\n",
			"\n\t\t"
		),
		CONCAT_LINE_STRING_CONSTRUCT(
			"\t\t",
			"\t\t\t",
			":",
			": ",
			"\n",
			"\n\t\t\t"
		),
		CONCAT_LINE_STRING_CONSTRUCT(
			"\t\t\t",
			"\t\t\t\t",
			":",
			": ",
			"\n",
			"\n\t\t\t\t"
		),

		// 3-7
		CONCAT_LINE_STRING_CONSTRUCT(
			"\t\t\t\t",
			"\t\t\t\t\t",
			":",
			": ",
			"\n",
			"\n\t\t\t\t\t"
		),
		CONCAT_LINE_STRING_CONSTRUCT(
			"\t\t\t\t\t",
			"\t\t\t\t\t\t",
			":",
			": ",
			"\n",
			"\n\t\t\t\t\t\t"
		),
		CONCAT_LINE_STRING_CONSTRUCT(
			"\t\t\t\t\t\t",
			"\t\t\t\t\t\t\t",
			":",
			": ",
			"\n",
			"\n\t\t\t\t\t\t\t"
		),
		CONCAT_LINE_STRING_CONSTRUCT(
			"\t\t\t\t\t\t\t",
			"\t\t\t\t\t\t\t\t",
			":",
			": ",
			"\n",
			"\n\t\t\t\t\t\t\t\t"
		)
	}
};
