import pygame
import random

def main():
    pygame.init()
    screen = pygame.display.set_mode((400,400))
    run = True
    clock = pygame.time.Clock()
    x,y = .5,3.8
    while run:
        for e in pygame.event.get():
            if e.type==pygame.QUIT:
                run = False
        
        print(x)
        print(y)
        x = y*x*(1-x)
        screen.fill((0,0,0))
        screen.set_at((int(200+x*100),200),(255,255,255))
        pygame.display.flip()
        clock.tick_busy_loop(30)
        
if __name__=="__main__":
    main()