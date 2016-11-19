#!/usr/bin/perl -w

use strict ;
use warnings;
package CWarn;
use Carp;
#use File::Slurp;
use Data::Dumper;
#use Tree::DAG_Node;
use List::MoreUtils qw(uniq);


#Score : | E1’ ∩ E2’ | / max(|E1’|, |E2’|) 
my $dirname = $ARGV[0];
    my %hash_token_number_to_name;
    my $output_sentence = "";
#opendir DIR, $dirname or die "cannot open dir $dirname: $!";
#my @filenames= readdir DIR;
#closedir(DIR);
{
    ` rm -f corpus_train_code.txt`;
    my $o_filename = 'corpus_train_code.txt';
    open(my $output_file, '>>', $o_filename) or die "Could not open file '$o_filename' $!";
    foreach my $filename (glob("$dirname/*.data")) {

#    my $filename = 'data.txt';
        print "\n working with ::$filename";
        if (open(my $fh, '<:encoding(UTF-8)', $filename)) {
            while (my $row = <$fh>) {
                chomp $row;
                my $line = $row;
                my @tokens = split / /,$line;
                if (@tokens == 1 ) {#new function found
#print "\n calling construct pair with ::". Dumper (%hash_token_number_to_name);
#print "\n working for function $tokens[0]";
                    construct_dataflowgraph(%hash_token_number_to_name);
                    %hash_token_number_to_name = ();#reset the hash for new function
                        next;
                }

                @tokens = split /:/,$line;
#print "\n tokens are::\n";
#print Dumper \@tokens;
                #print "\n id:: $tokens[0] and defs are $tokens[-1]";
#print "\n tokens are::\n";
#print Dumper \@tokens;

                my $token_number = trim($tokens[0]);
                my $token_parents = "";
                $token_parents = trim($tokens[-1]) unless  (@tokens < 3);
                
                my $token_name  = "";
                for(my $i=1;$i< scalar(@tokens)-1 ; $i++ ) {
                    $token_name = $token_name."_".remove_all_spaces($tokens[$i]);
                }
                next    if ($token_name eq '_skip' );
                $hash_token_number_to_name{$token_number}->{name}=$token_name;
                $hash_token_number_to_name{$token_number}->{parents}=$token_parents;
                #$hash_token_number_to_name{$token_number}->{finished}=-1;
# $hash_token_number_to_name{$token_number}->{node}= Tree::DAG_Node->new;
#print "$row\n";
            }
     #print "\n calling construct pair with ::". Dumper (%hash_token_number_to_name);
                    construct_dataflowgraph(%hash_token_number_to_name);
     #       construct_pair(%hash_token_number_to_name);
            %hash_token_number_to_name = ();#reset the hash for new function
#print "\n hash is ::\n". Dumper \%hash_token_number_to_name;
        } else {
            warn "Could not open file '$filename' $!";
        }

   #     last;
    say $output_file $output_sentence;
    say $output_file     "####$filename";
    $output_sentence = "";
    }
#print "\n final sentenece :: \n$output_sentence";
    close $output_file;
}
sub node_has_no_parent {
    my $node_num = shift;
    return 1 unless (exists $hash_token_number_to_name{$node_num});
    my $value = $hash_token_number_to_name{$node_num};
    my @parents = split / /,$value->{parents};
    return 1        if (scalar(@parents) == 0) ;
    foreach (@parents) {
        return 0 if (exists $hash_token_number_to_name{$_});
    }
    return 1;
}
sub mark_node_in_progress {
    my $node_num = shift;
    return  unless (exists $hash_token_number_to_name{$node_num});
    my $value = $hash_token_number_to_name{$node_num};
    $value->{finished} = 0;
}

sub mark_node_finished {
    my $node_num = shift;
    return  unless (exists $hash_token_number_to_name{$node_num});
    my $value = $hash_token_number_to_name{$node_num};
    $value->{finished} = 1;
}
sub node_unvisited {
    my $node_num = shift;
    return 0 unless (exists $hash_token_number_to_name{$node_num});
    my $value = $hash_token_number_to_name{$node_num};
    return 1 if ($value->{finished} == -1);
    return 0;
}
sub get_node_parents      {
    my $node_num = shift;
    my $value = $hash_token_number_to_name{$node_num};
    my @tokens = split / /,$value->{parents};
    my @parents;
    foreach (@tokens) {
      next unless exists $hash_token_number_to_name{$_};
    push @parents, $_;
    }
    return @parents;
}
sub iterative_dfs() {
    my ($node_num, $path_string) = @_;
    my @stack_dfs;
    my %visited_hash ;
    push @stack_dfs, $node_num;
    while (scalar(@stack_dfs)) {
        my $current_node = pop @stack_dfs;
        next if (exists $visited_hash{$current_node}) ;
        $visited_hash{$current_node} = 1;
        my @parents = get_node_parents($current_node );
        push @stack_dfs, @parents;

    }
}
sub depth_first_search {
    my ($node_num, $path_string) = @_;
    if (node_has_no_parent($node_num) ){
        return unless (scalar (split / /,$path_string ) > 1);
        #print "\n with string :: $path_string";
        $output_sentence = $output_sentence . " " . $path_string."\n";
        return    ;
    }
    mark_node_in_progress($node_num);
    my @parents = get_node_parents($node_num);
    my $node_name = $hash_token_number_to_name{$node_num}->{name};
    $path_string = $node_name ." ". $path_string if ($node_name ne 'unsupported'  );
    foreach my $parent_num (@parents) {
        depth_first_search ($parent_num, $path_string ) if (node_unvisited($parent_num));
    }
    mark_node_finished($node_num);
    #print "\nFinished:: $node_num ";
    
#    $output_sentence = $output_sentence . " " . $node_num . "\n";
}
sub start_dfs_forest {
    #print "\n got hash ::".Dumper(%hash_token_number_to_name);
    while (my ($number,$value) = each %hash_token_number_to_name ) {
        my $str_s = "";
        depth_first_search($number, $str_s)        if (node_unvisited($number));
    }
}
sub  trim { my $s = shift; $s =~ s/^\s+|\s+$//g; return $s };
sub  remove_all_spaces { my $s = shift; $s =~ s/\s+|\.//g; return $s };

sub construct_dataflowgraph {  
    my (%hash_nodes) = @_;
    my $sentence = "";
    my $d_sen = "";
    #print "\n hash node is ::".Dumper(%hash_nodes);
    my %parent = ();
    my %children = ();
    my %node_num_to_name;
    while (my ($number,$value) = each %hash_nodes ) {
        $node_num_to_name{$number} = $value->{name};
        my @parents_nodes = split / /,$value->{parents};
        @parents_nodes = grep {$_ > 0} @parents_nodes;
        #print "\n$number has parent nodes::@parents_nodes";
        push (@{$parent{$number}}, @parents_nodes ) if (@parents_nodes);
        foreach (@parents_nodes) {
            push (@{$children{$_}}, $number);
        }
    }
    my @all_nodes = sort { $a <=> $b } (uniq (keys %parent, keys %children));
    my @leaf_nodes = grep {!exists $children{$_}} @all_nodes;

    {
        my @stack = ();
        my %visited = ();
        my @path ;
        my @parent_size;
        foreach my $leaf (@leaf_nodes){
            push @stack, $leaf;
            while (scalar(@stack)) {
                my $node = pop @stack;
                push @path, $node;
                --$parent_size[-1] if (@parent_size) ;
                    next if (exists $visited{$node});
                $visited{$node} = 1;
                my @unvisited_parents = (grep {!exists $visited{$_} } @{$parent{$node}} );
                push @stack,@unvisited_parents ;
                if (@unvisited_parents) {
                    push @parent_size , scalar(@unvisited_parents) ;
                } else {
                    $d_sen = $d_sen . "\n ". join (" ", @path);

                    $sentence = $sentence ."\n". join(" ", @node_num_to_name{ grep {exists $node_num_to_name{$_} } @path});
                    pop @path;
                    while (scalar(@parent_size)  && $parent_size[-1] <=0 ) {
                        pop @path;
                        pop @parent_size;
                    }
                    
                }
#                print "\n now stack is ::".Dumper(@stack);
            }
        }
    }
    #print "\n leaf nodes are ::".Dumper(@leaf_nodes);
    #print "=====================----------------";
   # foreach my $n ( keys %parent ) {
   #     print "\n parent of $n are ". Dumper( @{$parent{$n}});
   # }
   # foreach my $n ( keys %children ) {
   #     print "\n children of $n are ". Dumper( @{$children{$n}});
   # }
   #print "$d_sen";
    #print "$sentence";
    #print "\n Constructed the graph ::".Dumper(%parent). " and the children::". Dumper(%children);
    $output_sentence = $output_sentence . $sentence;
}


sub construct_pair {  
    my (%hash) = @_;
    start_dfs_forest();
    return;
    construct_sentences(%hash);
    return;
    #print "\n got hash ::".Dumper(%hash);
    while (my ($number,$value) = each %hash ) {
#print "\n hash printing::". Dumper ($value);
        my $prefix_str = $value->{name};
        my @parents = split / /,$value->{parents};
        foreach (@parents) {
            next unless exists  $hash{$_};
            my $parent_name = $hash{$_}->{name};
           # $hash{$_}->{node}->add_daughter($value->{node});
            my $edge = $prefix_str . ' ' . $parent_name;
            #print "\n Edge is ::$edge";
            $output_sentence = $output_sentence . $edge . "\n";
        }
    }
}

sub construct_sentences {
    my (%hash) = @_;
    #print "\n got hash ::".Dumper(%hash);
    while (my ($number,$value) = each %hash ) {
    my $children_path_str = "";
        print "\n starting traversal from node number::$number";
        traverse_dataflow_path($children_path_str,$number, %hash) if ($value->{finished} == 0) ; 
        last;
    }
}
sub traverse_dataflow_path {  
    my ($children_path_str, $current_node_number,%hash ) = @_;
    print "\n called with $children_path_str, $current_node_number \n";
    #print "\n got hash ::".Dumper(%hash);
#print "\n hash printing::". Dumper ($value);
        my $value = $hash{$current_node_number};
        my @parents = split / /,$value->{parents};
        if ($hash{$current_node_number}->{finished} == 1 || scalar(@parents) == 0) {
            $output_sentence = $output_sentence ." ". $value->{name} ." ". $children_path_str . "\n"  ;
            $hash{$current_node_number}->{finished} = 1;
            return;
        }
        my $append_str = $value->{name};
        foreach (@parents) {
            next unless exists  $hash{$_};
            next if ($_ eq $current_node_number);
            my $parent_number = $_;
            my $parent_name = $hash{$parent_number}->{name};
            $children_path_str = $parent_name." " . $children_path_str;
            #print "\n NUMBER FROM INSIDE::$parent_number $parent_name";
            traverse_dataflow_path($children_path_str,$parent_number, %hash);
           # $hash{$_}->{node}->add_daughter($value->{node});
        }
        $hash{$current_node_number}->{finished} = 1;
        $output_sentence = $output_sentence ." ". $value->{name} ." ". $children_path_str . "\n"  ;
}
sub get_code_files {
    my $path    = shift;
    my $ONE_DAY = 86400; # seconds

    opendir (DIR, $path)
        or die "Unable to open $path: $!";

    my @files =
        map { $path . '/' . $_ }
        grep { !/^\.{1,2}$/ }
        readdir (DIR);

    # Rather than using a for() loop, we can just
    # return a directly filtered list.
    return
        grep { (/\.cil$/) 
                &&
               (! -l $_) }
        map { -d $_ ? get_code_files ($_) : $_ }
        @files;
}
