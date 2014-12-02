'''
Created on Jul 12, 2014

@author: jeremyblythe
'''
import pygame
from pygame.locals import *
import os
from time import sleep
import RPi.GPIO as GPIO
import sys, pygame

setup fifo
path = "uififo.fifo"
os.mkfifo(path)

#Setup the GPIOs as outputs - only 4 and 17 are available
GPIO.setmode(GPIO.BCM)
GPIO.setup(4, GPIO.OUT)
GPIO.setup(17, GPIO.OUT)

#Colours
WHITE = (255,255,255)

os.putenv('SDL_FBDEV', '/dev/fb1')
os.putenv('SDL_MOUSEDRV', 'TSLIB')
os.putenv('SDL_MOUSEDEV', '/dev/input/touchscreen')

pygame.init()
pygame.mouse.set_visible(False)
lcd = pygame.display.set_mode((320, 240))
lcd.fill((0,0,0))
pygame.display.update()

font_big = pygame.font.Font(None, 20)

touch_buttons = {'sellect':(160,140)} #, '4 @ 1/2 sec':(240,40), '17 @ 1 sec':(80,140), '4 @ 1 sec':(240,140), 'all':(160,220)

for k,v in touch_buttons.items():
    text_surface = font_big.render('%s'%k, True, WHITE)
    rect = text_surface.get_rect(center=v)
    lcd.blit(text_surface, rect)

pygame.display.update()

while True:
    # Scan touchscreen events
    for event in pygame.event.get():
        if(event.type is MOUSEBUTTONDOWN):
            pos = pygame.mouse.get_pos()
            print pos
        elif(event.type is MOUSEBUTTONUP):
            pos = pygame.mouse.get_pos()
            print pos
            #Find which quarter of the screen we're in
            x,y = pos
            if y < 240: #top half
                if x < 320: #left half
                    GPIO.output(17, True)
		    		#sleep(.5)
		    		#GPIO.output(17, False)
		   			fifo = open(path, "w")
		   			fifo.write("n")
		   			fifo.close()
                #else: #right half
                    #GPIO.output(4, True)
		    #sleep(.5)
		    #GPIO.output(4, False)
            #elif 100 < y <200: #bottom half
                #if x < 160: #left half
		    #GPIO.output(17, True)
		    #sleep(1)
                    #GPIO.output(17, False)
                #else: #right half
		    #GPIO.output(4, True)
		    #sleep(1)
                    #GPIO.output(4, False)
    	    #else:
		#GPIO.output(17, True)
		#GPIO.output(4, True)
		#sleep(1)
		#GPIO.output(17, False)
		#GPIO.output(4, False)
#	if event.type == KEYDOWN:
#	     if event.key == K_ESCAPE:
	        sys.exit()
    sleep(0.1)
