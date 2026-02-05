import pandas as pd
import matplotlib.pyplot as plt

def plot_simulation():
    # 1. Load Data
    try:
        df = pd.read_csv("simulation_data.csv")
    except FileNotFoundError:
        print("Error: 'simulation_data.csv' not found. Run the C++ simulation first.")
        return

    # 2. Setup Plot (2 Rows)
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 10), sharex=True)
    
    # --- Top Panel: Price & Inventory ---
    color = 'tab:blue'
    ax1.set_ylabel('Price ($)', color=color)
    ax1.plot(df['Step'], df['MidPrice'], label='Market Mid Price', color=color, linewidth=2)
    ax1.plot(df['Step'], df['MM_ReservationPrice'], label='MM Reservation Price', color='cyan', linestyle='--', alpha=0.7)
    ax1.tick_params(axis='y', labelcolor=color)
    ax1.legend(loc='upper left')
    ax1.grid(True, alpha=0.3)

    # Secondary Axis for Inventory
    ax1_right = ax1.twinx()
    color = 'tab:gray'
    ax1_right.set_ylabel('MM Inventory (Shares)', color=color)
    ax1_right.fill_between(df['Step'], df['MM_Inventory'], color=color, alpha=0.1, label='Inventory')
    ax1_right.tick_params(axis='y', labelcolor=color)

    ax1.set_title('Market Stability Analysis: Price vs. Inventory Response', fontsize=14)

    # --- Bottom Panel: Toxicity (VPIN) ---
    color = 'tab:red'
    ax2.set_xlabel('Simulation Step')
    ax2.set_ylabel('VPIN (Toxicity)', color=color)
    ax2.plot(df['Step'], df['VPIN'], color=color, linewidth=2, label='VPIN Score')
    ax2.tick_params(axis='y', labelcolor=color)
    ax2.grid(True, alpha=0.3)
    
    # Add Threshold Line
    ax2.axhline(y=0.6, color='black', linestyle=':', label='Toxicity Threshold (0.6)')
    ax2.legend(loc='upper left')
    
    ax2.set_title('RegTech Module: Toxicity Detection (VPIN)', fontsize=14)

    # Highlight the Crash Zone
    # Assuming crash starts at step 50
    ax1.axvline(x=50, color='orange', linestyle='-', alpha=0.5)
    ax2.axvline(x=50, color='orange', linestyle='-', alpha=0.5)
    ax1.text(52, df['MidPrice'].max(), "CRASH START", color='orange', fontweight='bold')

    plt.tight_layout()
    plt.savefig("niw_evidence_chart.png")
    print("Chart saved as 'niw_evidence_chart.png'")
    plt.show()

if __name__ == "__main__":
    plot_simulation()
