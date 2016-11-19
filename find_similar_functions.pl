#!/usr/bin/perl -w

use strict ;
use warnings;
package CWarn;
use Carp;
#use File::Slurp;
use Data::Dumper;
#use Tree::DAG_Node;
use List::MoreUtils qw(uniq);


my $dirname = $ARGV[0];
my $sorted_class_filename = $ARGV[1];
my %hash_token_number_to_name;
my $output_sentence = "";
my %word_2_class_map;
my %fileName_2_nodes_map;
#opendir DIR, $dirname or die "cannot open dir $dirname: $!";
#my @filenames= readdir DIR;
#closedir(DIR);
{
    read_init_word_class();
    #` rm -f corpus_train_code.txt`;
    #my $o_filename = 'corpus_train_code.txt';
    #open(my $output_file, '>>', $o_filename) or die "Could not open file '$o_filename' $!";

    my @selected_filenames;
    my $f_name='f1';
    if (open(my $fh, '<:encoding(UTF-8)', $f_name)) {
        while (my $row = <$fh>) {
            chomp $row;
            push @selected_filenames, $row;
        }
    }
    
#    foreach my $filename (glob("$dirname/*.data")) 
    foreach my $filename (uniq @selected_filenames) {
        my $function_name = "";
        my $newTag = $filename;

#    my $filename = 'data.txt';
        print "\n working with ::$filename";
        if (open(my $fh, '<:encoding(UTF-8)', $filename)) {
            my $function_name = "";
            while (my $row = <$fh>) {
                chomp $row;
                my $line = $row;
                my @tokens = split / /,$line;
                if (@tokens == 1 ) {#new function found
                    if (exists $fileName_2_nodes_map{$filename}->{$function_name}) {
                        my $tmp = $fileName_2_nodes_map{$filename}->{$function_name};
                        my %hash_tmp = %$tmp;
                        if (scalar(keys %hash_tmp ) < 10) {
                            delete $fileName_2_nodes_map{$filename}->{$function_name};
                            #print "\n After deleting map is ::".Dumper(%fileName_2_nodes_map);
                        }
                    }
                    $function_name = $tokens[0];
                    next;
                }
                @tokens = split /:/,$line;
                my $token_number = trim($tokens[0]);
                my $token_parents = trim($tokens[-1]);
                my $token_name  = "";
                for(my $i=1;$i< scalar(@tokens)-1 ; $i++ ) {
                    $token_name = $token_name."_".remove_all_spaces($tokens[$i]);
                }
                next unless (exists $word_2_class_map{$token_name});
                my $class_n = $word_2_class_map{$token_name};
                $fileName_2_nodes_map{$filename}->{$function_name}->{$class_n} = 1;
            }
        } else {
            warn "Could not open file '$filename' $!";
        }
    }
    find_similar_functions();
    #print "\n filename to class set::".Dumper(%fileName_2_nodes_map);
}
sub find_similar_functions {
    my %similar_function_found;
    my %similar_function_notfound;
    my %similarity_score_pair_files;
    foreach my $this_file (keys %fileName_2_nodes_map) {
        my $a_tokens_in_this_file = $fileName_2_nodes_map{$this_file};
        my %tokens_in_this_file = %$a_tokens_in_this_file;
        foreach my $other_file (keys %fileName_2_nodes_map) {
            next if ($other_file eq $this_file);
            my $pair_files = $other_file . $this_file ;
            next if (exists $similarity_score_pair_files{$pair_files});
            my $a_tokens_in_other_file = $fileName_2_nodes_map{$other_file};
            my %tokens_in_other_file = %$a_tokens_in_other_file;
            foreach my $func_this_file (keys %tokens_in_this_file ) {
                my $tmp = $tokens_in_this_file{$func_this_file};
                my %tokens_in_this_func = %$tmp;
                next if (exists $similar_function_found{$func_this_file});
                $similar_function_notfound{$func_this_file} = 1 unless (exists $similar_function_found{$func_this_file});
                foreach my $func_other_file (keys %tokens_in_other_file ) {
                next if (exists $similar_function_found{$func_other_file});
                    my $tmp2 = $tokens_in_other_file{$func_other_file};
                    my %tokens_in_other_func = %$tmp2;
                    my $diff_size =abs( scalar(keys %tokens_in_other_func) - scalar(keys %tokens_in_this_func));
                    next if ($diff_size > 10) ;
                    my %intersection = map { $_ => 1 } grep $tokens_in_this_func{$_}, keys %tokens_in_other_func;
                    my $similarity_score = scalar(keys %intersection)/ (scalar(keys %tokens_in_other_func) > scalar(keys %tokens_in_this_func) ?
                            scalar(keys %tokens_in_other_file) : scalar(keys %tokens_in_this_func));
                    my $pair_functions = $func_this_file . $func_other_file;
                    $similarity_score_pair_files{$pair_files}->{$pair_functions} = $similarity_score;
                    if ($similarity_score eq 1) {
                     delete $similar_function_notfound{$func_this_file} ;
                     $similar_function_found{$func_this_file}=1 ;
                     $similar_function_found{$func_other_file}=1 ;
                        print "\n $this_file::$func_this_file similar to $other_file::$func_other_file";
                    }

                }
            }
        }
    }
    print "\n Similar not found::".Dumper(keys %similar_function_notfound);
    print "\n printing similarity score" . Dumper(%similarity_score_pair_files );
#    foreach my $pair (sort { $similarity_score_pair_files{$a} <=> $similarity_score_pair_files{$b} } keys
#    %similarity_score_pair_files) {
#        printf "%-8s %s\n", $pair, $similarity_score_pair_files{$pair};
#    }
}
sub read_init_word_class
    {
        my $class_file;
        my $class_file_name = $sorted_class_filename      ;
        open($class_file, '<:encoding(UTF-8)', $class_file_name) or die "could not find class file"; 
        while (my $row = <$class_file>) {
            chomp $row;
            my @tokens = split / /, $row;
            my $class_num = $tokens[1];
            my $vocab_word = $tokens[0];
            $word_2_class_map{$vocab_word} = $class_num;
        }
        #print "\n class map is::".Dumper(%word_2_class_map);


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
sub depth_first_search {
    my ($node_num, $path_string) = @_;
    if (node_has_no_parent($node_num) ){
        print "\n with string :: $path_string";
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
