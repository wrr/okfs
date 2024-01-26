"if type perl >/dev/null; then\n"
"	perl <<-\"EOF\"\n"
"		if ($] lt 5) {\n"
"			print(\"failed\\n\");\n"
"			exit(1);\n"
"		}\n"
"		eval \"use Fcntl; use IO::File; use POSIX qw(mktime);\";\n"
"		if ($@) {\n"
"			print(\"failed\\n\");\n"
"			exit(1);\n"
"		}\n"
"		my $ouid = $>;\n"
"		print(\"ok stable\");\n"
"		$>++;\n"
"		print(\" preserve\") if ($ouid + 1 == $>);\n"
"		print(\"\\n\");\n"
"EOF\n"
"else\n"
"	echo failed;\n"
"fi;\n"