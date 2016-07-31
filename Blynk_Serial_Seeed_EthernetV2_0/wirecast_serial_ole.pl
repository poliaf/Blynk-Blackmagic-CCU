#!/usr/bin/perl

use Win32::OLE;
use Win32::SerialPort;
use 5.18.0;
use warnings;

my $numOfArguments = @ARGV;

if ($numOfArguments != 1){
  die "please provide serial port number of arduino as an argument\n";
}

# Set up the serial port with argument
chomp (my $portNumber = pop @ARGV);
my $port = Win32::SerialPort->new("\\\\.\\COM".$portNumber)
  or die "can't open serial port to arduino\n";
  
$port->baudrate(9600); 
$port->parity("none");
$port->databits(8); 
$port->stopbits(1); 
$port->debug(0);
$port->handshake('none');

# define line termination for $port->lookfor()
# Note: Arduino serial.println terminates each line/string sent with a "\r\n"

$port->are_match("\r\n");

my $data;

# getting the wirecast object
my $wirecast = Win32::OLE->GetActiveObject("Wirecast.Application")
  or die "Please open Wirecast and set up all camera shots before running this script\n";

if ($wirecast) {
  
  # an example of calling a method:
  my $doc = $wirecast->DocumentByIndex(1);
  
  if ($doc) {
    my $overlayLayer = $doc->LayerByName("overlay");
    my $normalLayer = $doc->LayerByName("normal");
    my $underlayLayer = $doc->LayerByName("underlay");
    my @overlayShots; # array of shot IDs
    my @normalShots; # array of shot IDs
    my @underlayShots; # array of shot IDs
    my %cameraShots; # hashmap of shot IDs and camera number
    my %cameraStatus; # hashmap of cameras and their status (1 for live, 2 for preview, 0 for neither)
    
    if ($overlayLayer) {
      my $overlayShotCount = $overlayLayer->ShotCount();
      say $overlayShotCount, " shots found in overlay layer.";
      for (my $i = 0; $i <= $overlayShotCount; ++$i){
        push @overlayShots, $overlayLayer->ShotIDByIndex($i);
        
      }
    }
    
    if ($normalLayer) {
      my $normalShotCount = $normalLayer->ShotCount();
      say $normalShotCount, " shots found in normal layer.";
      for (my $i = 0; $i <= $normalShotCount; ++$i){
        push @normalShots, $normalLayer->ShotIDByIndex($i);
      }
    }
    
    if ($underlayLayer) {
      my $underlayShotCount = $underlayLayer->ShotCount();
      say $underlayShotCount, " shots found in underlay layer.";
      for (my $i = 0; $i <= $underlayShotCount; ++$i){
        push @underlayShots, $underlayLayer->ShotIDByIndex($i);
      }
    }
    foreach my $shot (@overlayShots){
      my $currentShot = $doc->ShotByShotId($shot);
      if ($currentShot){
        print "What number camera is shot \"", $currentShot->{Name}, "\" in the overlay(2nd) layer: ";
        chomp (my $cameraNum = <STDIN>);
        if ($cameraNum > 0){
          $cameraShots{$shot} = $cameraNum;
          $cameraStatus{$cameraNum} = 0;
        }
      }
    }
    foreach my $shot (@normalShots){
      my $currentShot = $doc->ShotByShotId($shot);
      if ($currentShot){
        print "What number camera is shot \"", $currentShot->{Name}, "\" in the normal(3rd) layer: ";
        chomp (my $cameraNum = <STDIN>);
        if ($cameraNum > 0){
          $cameraShots{$shot} = $cameraNum;
          $cameraStatus{$cameraNum} = 0;
        }
      }
    }
    foreach my $shot (@underlayShots){
      my $currentShot = $doc->ShotByShotId($shot);
      if ($currentShot){
        print "What number camera is shot \"", $currentShot->{Name}, "\" in the underlay(4th) layer: ";
        chomp (my $cameraNum = <STDIN>);
        if ($cameraNum > 0){
          $cameraShots{$shot} = $cameraNum;
          $cameraStatus{$cameraNum} = 0;
        }
      }
    }
    my $numOfCams = keys %cameraShots;
    if ($numOfCams == 0){
      die "No Cameras: Exiting...\n";
    }
    while ($doc){
      $data = $port->lookfor();
      if ($data)
      {
         say $data;
      }
      while (my ($shot, $cameraNum) = each %cameraShots){
        my $currentShot = $doc->ShotByShotID($shot);
        my $currentStatus = $cameraStatus{$cameraNum};
        if ($currentShot->Live()){
          if ($currentStatus != 1){
            say "Camera ", $cameraNum, " is now Live";
            $port->lookclear;
            $port->write('C' . $cameraNum . 'L');

          }
          $cameraStatus{$cameraNum} = 1;
        }
        elsif ($currentShot->Preview()){
          if ($currentStatus != 2){
            say "Camera ", $cameraNum, " is now in Preview";
            $port->lookclear;
            $port->write('C' . $cameraNum . 'P');
          }
          $cameraStatus{$cameraNum} = 2;
        }
        else{
          if ($currentStatus != 0){
            $port->lookclear;
            $port->write('C' . $cameraNum . 'N');
          }
          $cameraStatus{$cameraNum} = 0;
        }
      }
	  sleep (1);
    }
  }
  say "Wirecast document closed";
  $port->close();
}