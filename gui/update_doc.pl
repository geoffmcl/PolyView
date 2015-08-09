#!/usr/bin/perl
use strict;        # insist that all variables be declared
use diagnostics;   # expand the cryptic warnings

MAIN:{

  # Take the documentation from a text file and insert it into a C++
  # file to be compiled and embedded with the executable.

  my $htmlDocFile = "documentation.html";
  my $cppDocFile  = "documentation.cpp";

  open(FILE, "<$htmlDocFile");
  my $preProcDocText = join("", <FILE>);
  close(FILE);

  # Make the image local and rm XEmacs's autogenerated html tag
  $preProcDocText =~ s/\"[^\"]*?pvLogo.png.*?\"/:pvLogo.png/g;
  $preProcDocText =~ s/Updated:.*?$//sg;

  # Other minor
  my $local = 1; # If the code does not compile, update and set this to 0.
  if ($local){
    $preProcDocText =~ s/\s*\<br\>\s*\<br\>(^|\n).*?source code.*?\n/\n/g;
    $preProcDocText =~ s/(^|\n).*?mailto:.*?\n/\n/g;
    $preProcDocText =~ s/free and open source software program for Linux/program/g;
    $preProcDocText =~ s/\<h2\>Download.*?(\<h2\>)/$1/sg;
  }

  my $docText = "";
  foreach my $line ( split("\n", $preProcDocText)  ){
    $line =~ s/\"/\\\"/g; # Escape any quotes
    $docText .= '"' . $line . '\\n"' . "\n";
  }
  $docText =~ s/\s*$//g;

  open(FILE, "<$cppDocFile") || die "Cannot open file: $cppDocFile\n";
  my $text = join("", <FILE>);
  close(FILE);

  my $tag = 'char docText[] =' . $docText . ";\n";

  $text =~ s!(^.*?[ ]*//[ ]* Begin.*?\n).*?([ ]*//[ ]*End.*?)$! $1 . $tag . $2 !egs;
  $text =~ s/\s*$/\n/g;

  open(FILE, ">$cppDocFile");
  print FILE $text;
  close(FILE);

  if ($local){
    my $appFile = "appWindow.cpp";
    open(FILE, "<$appFile"); $text = join("", <FILE>); close(FILE);
    $text =~ s!\/*(help-\>insertItem\(\"About)!\/\/$1!g;
    open(FILE, ">$appFile");
    print FILE $text;
    close(FILE);
  }
}
