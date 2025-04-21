# Trading Bot Web Interface

A modern web interface for monitoring and analyzing trading bot performance, built with Next.js, Tailwind CSS, and Recharts.

## Features

- 📊 Real-time performance monitoring
- 📈 Interactive charts and visualizations
- 📋 Detailed trade analysis
- ⚙️ Configuration management
- 🎨 Dark mode support
- 📱 Responsive design

## Pages

### Overview (`/`)
- Equity curve visualization
- Key performance indicators
- Total profit, Sharpe ratio, win rate, and max drawdown

### Trades (`/trades`)
- Detailed trade history table
- PnL distribution chart
- Trade type and performance filters

### Stats (`/stats`)
- Advanced performance metrics
- Sharpe ratio gauge
- PnL distribution analysis
- Zoomable equity curve

### Config (`/config`)
- Strategy configuration details
- Parameter visualization
- Execution information

## Getting Started

### Prerequisites

- Node.js 18.0.0 or later
- npm or yarn

### Installation

1. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/tradingbot-web.git
   cd tradingbot-web
   ```

2. Install dependencies:
   ```bash
   npm install
   # or
   yarn install
   ```

3. Create a `public/export` directory and add your JSON files:
   - `results.json`: Global metrics
   - `trades.json`: Trade history
   - `equity.json`: Equity curve data
   - `config.json`: Strategy configuration

4. Start the development server:
   ```bash
   npm run dev
   # or
   yarn dev
   ```

5. Open [http://localhost:3000](http://localhost:3000) in your browser.

## Data Format

### results.json
```json
{
  "totalProfit": 10000,
  "sharpeRatio": 1.5,
  "winRate": 0.65,
  "maxDrawdown": 0.15,
  "totalTrades": 100,
  "averagePnl": 100,
  "bestTrade": 500,
  "worstTrade": -200
}
```

### trades.json
```json
[
  {
    "timestamp": "2024-01-01T00:00:00Z",
    "type": "buy",
    "price": 100.0,
    "volume": 1.0,
    "pnl": 10.0
  }
]
```

### equity.json
```json
[
  {
    "timestamp": "2024-01-01T00:00:00Z",
    "value": 10000.0
  }
]
```

### config.json
```json
{
  "strategy": "Momentum",
  "parameters": {
    "window_size": 20,
    "threshold": 0.02
  },
  "executionTime": "2024-01-01T00:00:00Z",
  "randomSeed": 12345
}
```

## Development

### Project Structure

```
src/
├── app/                    # Next.js app router pages
│   ├── page.tsx           # Home page
│   ├── trades/            # Trades page
│   ├── stats/             # Stats page
│   └── config/            # Config page
├── components/            # Reusable components
│   ├── ui/               # UI components
│   ├── charts/           # Chart components
│   └── layout/           # Layout components
└── lib/                  # Utility functions
```

### Adding New Features

1. Create a new page in `src/app/`
2. Add components in `src/components/`
3. Update the sidebar navigation in `src/components/layout/Sidebar.tsx`

## Deployment

The project can be deployed to any platform that supports Next.js applications, such as Vercel, Netlify, or self-hosted servers.

### Vercel Deployment

1. Push your code to a GitHub repository
2. Import the project in Vercel
3. Configure environment variables if needed
4. Deploy

## Contributing

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- [Next.js](https://nextjs.org/)
- [Tailwind CSS](https://tailwindcss.com/)
- [Recharts](https://recharts.org/)
- [Lucide Icons](https://lucide.dev/)
