# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(my-test2) begin
(my-test2) open "sample.txt"
(my-test2) end
my-test2: exit(0)
EOF
pass;
