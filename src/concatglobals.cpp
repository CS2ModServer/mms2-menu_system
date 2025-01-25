#include <array>

#include <concat.hpp>

#define CONCAT_LINE_STRING_CONSTRUCT(head, starts, before, between, ends, endsAndStartsWith) {{{{head, starts, before, between, ends, endsAndStartsWith}}}}

const std::array<const CConcatLineString, 8> g_arrEmbedsConcat = 
{
	{
		// 0-3
		CONCAT_LINE_STRING_CONSTRUCT
		(
			"",     // Heads with.
			"\t",   // Starts with.
			":",    // Before key.
			": ",   // Between key & value.
			"\n",   // Ends.
			"\n\t"  // Ends and starts of next line.
		),
		CONCAT_LINE_STRING_CONSTRUCT
		(
			"\t",
			"\t\t",
			":",
			": ",
			"\n",
			"\n\t\t"
		),
		CONCAT_LINE_STRING_CONSTRUCT
		(
			"\t\t",
			"\t\t\t",
			":",
			": ",
			"\n",
			"\n\t\t\t"
		),
		CONCAT_LINE_STRING_CONSTRUCT
		(
			"\t\t\t",
			"\t\t\t\t",
			":",
			": ",
			"\n",
			"\n\t\t\t\t"
		),

		// 3-7
		CONCAT_LINE_STRING_CONSTRUCT
		(
			"\t\t\t\t",
			"\t\t\t\t\t",
			":",
			": ",
			"\n",
			"\n\t\t\t\t\t"
		),
		CONCAT_LINE_STRING_CONSTRUCT
		(
			"\t\t\t\t\t",
			"\t\t\t\t\t\t",
			":",
			": ",
			"\n",
			"\n\t\t\t\t\t\t"
		),
		CONCAT_LINE_STRING_CONSTRUCT
		(
			"\t\t\t\t\t\t",
			"\t\t\t\t\t\t\t",
			":",
			": ",
			"\n",
			"\n\t\t\t\t\t\t\t"
		),
		CONCAT_LINE_STRING_CONSTRUCT
		(
			"\t\t\t\t\t\t\t",
			"\t\t\t\t\t\t\t\t",
			":",
			": ",
			"\n",
			"\n\t\t\t\t\t\t\t\t"
		)
	}
};

const CConcatLineString g_aMenuConcat = CONCAT_LINE_STRING_CONSTRUCT
(
	"",     // Heads with.
	"",     // Starts with.
	"",     // Before key.
	". ",   // Between key & value.
	"\n",   // Ends.
	"\n\n"  // Ends every title.
);
