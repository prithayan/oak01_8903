#!/usr/bin/python
from scipy.cluster.vq import kmeans2
from sklearn.cluster import KMeans
from gensim.models import Word2Vec
import logging
from sys import argv
import numpy as np

#script, filename = argv
#filename_clang_func_defuse = '/home/pbarua/scripts/run_dir_tmp/func_defuse_lines.txt'
#
#clang_func_defuse_file = open(filename_clang_func_defuse, 'r');
#
#print "Here's your file %r:" % clang_func_defuse_file
##defuse_list = []
#func_2_defuse_mat = []
#defuse_for_one_func = []
#first_line=1
#for defuse_line in clang_func_defuse_file:
#    defuse_line = defuse_line.strip(' \t\n\r')
#    if defuse_line == "":
#        continue
#    if defuse_line.startswith('%') :
#        if first_line==0:
#            func_2_defuse_mat.append(defuse_for_one_func)
#        defuse_for_one_func = []
#        first_line=0
#    else:
#        #defuse_list.append(defuse_line)
#        defuse_for_one_func.append(defuse_line)
#func_2_defuse_mat.append(defuse_for_one_func)

#print "\n for funcname::%s"% func_list[2]
#print func_2_defuse_mat[2]
#print "\n for funcname::%s" %func_list[22]
#print func_2_defuse_mat[22]
#print "\n for funcname::%s"% func_list[12]
#print func_2_defuse_mat[12]


num_funcs=0
func_list = []
model = Word2Vec.load_word2vec_format('/home/pbarua/scripts/run_dir_7/word_vec.txt', binary=False)
vec_dim = model.layer1_size
for string_func in model.vocab:
    if "selectd_2" in string_func:
        func_list.append(string_func )
        num_funcs+=1 
function_vecs = np.zeros(( num_funcs, vec_dim ) )
row=0
for string_func in model.vocab:
    if "selectd_2" in string_func:
        function_vecs[row]=model[string_func]
        #print string_func
        #print function_vecs[row]
        row+=1
#print function_vecs[10]
#print func_list[10]
result_kmeans,kmeans_label = kmeans2(function_vecs, 50 )

i=0
for func_name in func_list:
    print func_name + " %d " % kmeans_label[i]
    i=i+1
#num_def_use_in_func = 0
##for defuse_str in defuse_list
#for row in range(len(func_2_defuse_mat)):
#    func_defuse_sum = np.zeros(vec_dim )
#    for col in range(len(func_2_defuse_mat[row] )):
#        defuse_str = func_2_defuse_mat[row][col]
#        if defuse_str in model.vocab:
#            func_defuse_sum +=model[defuse_str ]
#
#   # print "len is ::%d" % (len(func_2_defuse_mat[row] ) )
#    #print type (len(func_2_defuse_mat[row] ))
#    #print type (func_defuse_sum)
#    num_defuse = (len(func_2_defuse_mat[row] ))
#    #print  func_defuse_sum/ num_defuse
#    if num_defuse != 0:
#        function_vecs[row] = func_defuse_sum/ num_defuse
#    if (np.isnan(function_vecs[row]).any()):
#        print "NAN found"
#        print function_vecs[row]
#        break
#    if (np.isinf(function_vecs[row]).any()):
#        print "infs found"
#        print function_vecs[row]
#        break
#
#
#
#i=0
#for func_name in func_list:
#    print func_name + " %d " % kmeans_label[i]
##    print kmeans_label[i]
#    i=i+1
##print kmeans_label
##print func_list
#
#
##model['alloca_i16']


