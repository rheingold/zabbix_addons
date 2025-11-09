# Assets - JavaScript

**Purpose:** Client-side widget logic and rendering

**Files:**
- `class.widget.js` - Main widget class (extends CWidget), handles rendering, events, graph drawing
- `widget.edit.js` - Configuration form interactivity

**Architecture:**
- **class.widget.js** extends Zabbix's `CWidget` base class
- Implements lifecycle methods: `onActivate()`, `onFeedback()`, `setContents()`
- Handles all client-side graph rendering using HTML5 Canvas
- Manages user interactions (hover, resize)

**Data Flow:**
1. Server passes data via `views/widget.view.php` using `setVar('graph_data', ...)`
2. `setContents(response)` receives data as `response.graph_data`
3. `renderGraph()` draws graph on canvas
4. Event listeners handle user interactions

**Dependencies:**
- Extends: `CWidget` (Zabbix framework)
- Receives: Data from `includes/GraphData.php` via controller
- Interacts: HTML canvas, DOM elements
- Communicates: Back to server via `onFeedback()` for dashboard updates

**Browser Compatibility:**
Requires HTML5 Canvas support, modern JavaScript (ES6+)
