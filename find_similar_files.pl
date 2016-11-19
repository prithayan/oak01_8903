#!/usr/bin/perl -w

use strict ;
use warnings;
package CWarn;

#use File::Slurp;
use Data::Dumper;
#use Tree::DAG_Node;


my %filename_to_class;

my $filename = $ARGV[0];
if (open(my $fh, '<:encoding(UTF-8)', $filename)) {
    while (my $row = <$fh>) {
        chomp $row;
        my $line = $row;
        my @tokens = split / /,$line;
        if (@tokens != 2 ) {#new function found
            next;
        }
        my $func_name_file = $tokens[0];
        my $class = $tokens[1];
        @tokens = split /\//,$func_name_file;
        print "\n file name::". substr($tokens[-1],0,2);
        my $filename = substr($tokens[-1],0,2);
        $filename_to_class{$filename }->{$class} = 0  unless exists ( $filename_to_class{$filename }->{$class}) ;
        $filename_to_class{$filename }->{$class} += 1;
    }
} else {
    warn "Could not open file '$filename' $!";
}
print Dumper(%filename_to_class )
