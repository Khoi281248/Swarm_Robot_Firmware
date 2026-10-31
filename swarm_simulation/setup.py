import numpy as np

import matplotlib
import matplotlib.pyplot as plt
import matplotlib.animation as animation
print("Numpy version that being used in this project: 2.5.2")
print("This machine's numpy version:", np.__version__)


class SwarmSimulation:
    def __init__(self, n_bots: int = 1, ground_size: float = 1.0):
        self.ground_size = ground_size
        self.dt = 0.01
        self.t_ref = 0.0
        self.n_bots = n_bots

        # Initialize robots along the line
        self.robots = []
        x_i = ground_size / 10
        y_i = ground_size / 10
        theta_i = np.pi/2
        for i in range(n_bots):
            self.robots.append([x_i + ground_size*0.05*i, y_i, theta_i])
        self.robots = np.asarray(self.robots).T

    def update(self):
        self.t_ref += self.dt
        # Update random posture - implement algorithm in this
    
    def sim(self, fig, ax):
        # Scatter plot for robot positions and quiver for orientations (theta)
        scat = ax.scatter([], [], s=50, c='black', label='Robots')
        quiver = ax.quiver(
                np.empty(self.n_bots), np.empty(self.n_bots), 
                np.empty(self.n_bots), np.empty(self.n_bots), 
                color='red', scale=20, width=0.005)
        
        def init():
            scat.set_offsets(np.empty((0, 2)))
            return scat
        
        def animate(frame):
            self.update()
            
            # Extract positions and orientations
            x, y, theta = self.robots
            u = np.cos(theta)
            v = np.sin(theta)
            theta = np.degrees(theta)

            # Update scatter plot
            scat.set_offsets(np.column_stack([x, y]))
            
            # Update directional arrows (quiver)
            quiver.set_offsets(np.column_stack([x, y]))
            quiver.set_UVC(u, v)
            
            return scat, quiver
        
        ani = animation.FuncAnimation(fig, animate, init_func=init, frames=1000, interval=100, blit=False)
        
        ax.legend(loc='upper right')
        plt.show()


if __name__ == "__main__":
    # --- Visualization Setup ---
    GROUND_SIZE = 1.0
    sim = SwarmSimulation(n_bots=10, ground_size=GROUND_SIZE)
    fig, ax = plt.subplots(figsize=(6, 6))
    ax.set_xlim(0, GROUND_SIZE)
    ax.set_ylim(0, GROUND_SIZE)
    ax.set_aspect('equal')
    ax.set_title("Swarm Simulation")
    ax.set_xlabel("x"); ax.set_ylabel("y")
    sim.sim(fig, ax)
