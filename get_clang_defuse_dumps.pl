#!/usr/bin/perl -w

use strict ;
use warnings;
package CWarn;
#use Carp;
##use File::Slurp;
#use Data::Dumper;
##use Tree::DAG_Node;
#use List::MoreUtils qw(uniq);


my $dirname = $ARGV[0];
opendir DIR, $dirname or die "cannot open dir $dirname: $!";
my @filenames= readdir DIR;
#closedir(DIR);

    ` rm -f defuse_dump_file.txt`;
    #my $o_filename = 'corpus_train_code.txt';
    #open(my $output_file, '>>', $o_filename) or die "Could not open file '$o_filename' $!";

foreach my $filename (@filenames) {
    my $c_file = $dirname . '/' . $filename;

    if ($c_file =~ /\.c$/i) {
    print "working with file $c_file";
    `~/llvm/llvm/build/bin/clang -Xclang  -load -Xclang /home/pbarua/llvm/llvm/build/lib/LLVMHello.so -w -c -g -emit-llvm  $c_file`;
    } else {
    print "working with file $c_file";
    `~/llvm/llvm/build/bin/clang++ -Xclang -load -Xclang /home/pbarua/llvm/llvm/build/lib/LLVMHello.so -w -c -g -emit-llvm  $c_file`;
    }
    #`rm *.bc`;

}
