// Evidence Graph Engine (Cytoscape.js)
let cyInstance = null;
let currentGraphData = null;
let currentStartEntity = null;
let containerResizeObserver = null;

/**
 * Maps entity/event types to hex colors.
 * Types are normalized with String(type).toLowerCase().
 * - user -> #b58cff
 * - device -> #ffc24a
 * - file -> #5aa9ff
 * - event -> #ff6b6b
 * - network/ip/host -> #4ade80
 * - anything else -> #9ca3af
 */
function getNodeColor(type, kind) {
  const t = String(type || '').toLowerCase().trim();
  const k = String(kind || '').toLowerCase().trim();

  if (t === 'user' || k === 'user') return '#b58cff';
  if (t === 'device' || k === 'device') return '#ffc24a';
  if (t === 'file' || k === 'file') return '#5aa9ff';
  if (t === 'event' || k === 'event') return '#ff6b6b';
  if (
    t.includes('network') || t.includes('ip') || t.includes('host') ||
    k.includes('network') || k.includes('ip') || k.includes('host')
  ) {
    return '#4ade80';
  }
  return '#9ca3af';
}

/**
 * Reads colors from CSS variables (--ink, --paper, --soft, --txt)
 * to support both light and dark modes dynamically.
 */
function getThemeColors() {
  const style = getComputedStyle(document.documentElement);
  return {
    ink: style.getPropertyValue('--ink').trim() || '#2a12d6',
    paper: style.getPropertyValue('--paper').trim() || '#ffffff',
    soft: style.getPropertyValue('--soft').trim() || '#f6f3ff',
    txt: style.getPropertyValue('--txt').trim() || '#1d1260'
  };
}

/**
 * Places degree 0 (isolated/unconnected) nodes in a tidy grid
 * beneath the main connected component with 90px spacing.
 */
function positionIsolatedNodes(cy) {
  if (!cy) return;
  const isolated = cy.nodes().filter(n => n.degree() === 0);
  if (isolated.length === 0) return;

  const connected = cy.nodes().filter(n => n.degree() > 0);
  let startY = 120;
  let centerX = 0;

  if (connected.length > 0) {
    const bb = connected.boundingBox();
    startY = bb.y2 + 100;
    centerX = (bb.x1 + bb.x2) / 2;
  } else {
    const container = cy.container();
    if (container && container.clientWidth > 0) {
      centerX = container.clientWidth / 2;
      startY = Math.max(100, container.clientHeight / 4);
    }
  }

  const spacing = 90;
  let cols;
  if (isolated.length <= 2) {
    cols = isolated.length;
  } else if (isolated.length <= 6) {
    cols = 3;
  } else if (isolated.length <= 12) {
    cols = 4;
  } else {
    cols = Math.ceil(Math.sqrt(isolated.length));
  }

  isolated.forEach((node, idx) => {
    const row = Math.floor(idx / cols);
    const col = idx % cols;
    const totalInThisRow = Math.min(cols, isolated.length - row * cols);
    const x = centerX - ((totalInThisRow - 1) * spacing) / 2 + col * spacing;
    const y = startY + row * spacing;
    node.position({ x, y });
  });
}

/**
 * Runs the cose layout safely:
 * - Checks that container has non-zero width & height.
 * - Calls cy.resize() first, then layout.run(), then positions isolated nodes, then cy.fit(undefined, 40).
 */
function runCoseLayout(cy, onDone) {
  if (!cy) return;
  const container = cy.container();
  if (!container) return;

  const rect = container.getBoundingClientRect();
  if (rect.width === 0 || rect.height === 0 || container.offsetWidth === 0 || container.offsetHeight === 0) {
    requestAnimationFrame(() => runCoseLayout(cy, onDone));
    return;
  }

  cy.resize();
  const layout = cy.layout({
    name: 'cose',
    animate: false,
    fit: true,
    padding: 40,
    nodeRepulsion: () => 20000,
    idealEdgeLength: () => 120,
    edgeElasticity: () => 100,
    nodeOverlap: 30,
    gravity: 0.4,
    componentSpacing: 120,
    numIter: 2000,
    randomize: true
  });

  const finish = () => {
    positionIsolatedNodes(cy);
    cy.fit(undefined, 40);
    if (onDone) onDone();
  };

  layout.one('layoutstop', finish);
  layout.run();
  finish();
}

/**
 * Main drawGraph function.
 * Accepts:
 *   drawGraph(container, data, options)
 *   OR drawGraph(container, data, onNodeTap, onEdgeTap, onBackgroundTap)
 */
function drawGraph(container, data, options = {}) {
  if (!container || !window.cytoscape) return null;

  // Support legacy positional arguments
  if (typeof options === 'function') {
    options = {
      onNodeTap: options,
      onEdgeTap: arguments[3],
      onBackgroundTap: arguments[4]
    };
  }

  // Ensure minimum container height of 480px
  if (container.style) {
    if (!container.style.minHeight || parseInt(container.style.minHeight, 10) < 480) {
      container.style.minHeight = '480px';
    }
  }

  if (cyInstance) {
    try { cyInstance.destroy(); } catch (e) {}
    cyInstance = null;
  }

  currentGraphData = data;
  currentStartEntity = options.startEntity ? String(options.startEntity) : '';

  const colors = getThemeColors();
  const rawNodes = data.nodes || [];
  const rawEdges = data.edges || [];

  const elements = [
    ...rawNodes.map(n => {
      const isStart = currentStartEntity && (String(n.id) === currentStartEntity);
      let rawLabel = String(n.name || n.label || n.id || '');
      let displayLabel = rawLabel.length > 18 ? rawLabel.slice(0, 18) : rawLabel;
      return {
        data: {
          id: String(n.id),
          name: n.name || n.label || n.id,
          label: displayLabel,
          fullLabel: rawLabel,
          type: n.type || n.kind || 'unknown',
          kind: n.kind || '',
          color: getNodeColor(n.type, n.kind)
        },
        classes: isStart ? 'is-start' : ''
      };
    }),
    ...rawEdges.map(e => ({
      data: {
        id: String(e.record_id || `e_${e.source}_${e.target}_${Math.random()}`),
        source: String(e.source),
        target: String(e.target),
        label: e.relationship || e.label || '',
        relationship: e.relationship || e.label || '',
        rec: e.record_id || ''
      }
    }))
  ];

  cyInstance = cytoscape({
    container: container,
    elements: elements,
    style: [
      // Base Node
      {
        selector: 'node',
        style: {
          'width': 38,
          'height': 38,
          'background-color': 'data(color)',
          'border-width': 2,
          'border-color': colors.ink,
          'label': 'data(label)',
          'color': colors.ink,
          'font-size': 12,
          'font-weight': 600,
          'font-family': 'IBM Plex Sans, sans-serif',
          'text-valign': 'bottom',
          'text-margin-y': 6,
          'text-background-color': colors.paper,
          'text-background-opacity': 0.85,
          'text-background-padding': 2,
          'text-background-shape': 'roundrectangle',
          'text-wrap': 'ellipsis',
          'text-max-width': 90
        }
      },
      // Start Entity: thicker 4px border & larger size 48px
      {
        selector: 'node.is-start',
        style: {
          'width': 48,
          'height': 48,
          'border-width': 4,
          'border-color': colors.ink
        }
      },
      // Base Edge: curve-style bezier, width 2, arrow size 8, opacity 0.7, label hidden
      {
        selector: 'edge',
        style: {
          'curve-style': 'bezier',
          'width': 2,
          'line-color': colors.ink,
          'target-arrow-color': colors.ink,
          'target-arrow-shape': 'triangle',
          'arrow-scale': 1.0,
          'opacity': 0.7,
          'label': ''
        }
      },
      // Edge Label when active or hovered
      {
        selector: 'edge.show-label',
        style: {
          'label': 'data(label)',
          'font-size': 10,
          'font-family': 'IBM Plex Sans, sans-serif',
          'color': colors.ink,
          'text-rotation': 'autorotate',
          'text-background-color': colors.paper,
          'text-background-opacity': 0.85,
          'text-background-padding': 2,
          'text-background-shape': 'roundrectangle',
          'opacity': 1,
          'width': 2
        }
      },
      // Highlighted & Dimmed states
      {
        selector: 'node.highlighted',
        style: {
          'opacity': 1,
          'z-index': 99
        }
      },
      {
        selector: 'edge.highlighted',
        style: {
          'opacity': 1,
          'width': 2.5,
          'z-index': 98
        }
      },
      {
        selector: 'node.dimmed',
        style: {
          'opacity': 0.2
        }
      },
      {
        selector: 'edge.dimmed',
        style: {
          'opacity': 0.2
        }
      }
    ]
  });

  // Edge label reveal on hover
  cyInstance.on('mouseover', 'edge', e => {
    e.target.addClass('show-label');
  });
  cyInstance.on('mouseout', 'edge', e => {
    if (!e.target.hasClass('highlighted')) {
      e.target.removeClass('show-label');
    }
  });

  // Connected edge labels reveal on node hover
  cyInstance.on('mouseover', 'node', e => {
    e.target.connectedEdges().addClass('show-label');
  });
  cyInstance.on('mouseout', 'node', e => {
    e.target.connectedEdges().forEach(edge => {
      if (!edge.hasClass('highlighted')) {
        edge.removeClass('show-label');
      }
    });
  });

  // On node tap: highlight node and neighbors, dim rest to 0.2, fill #side
  cyInstance.on('tap', 'node', e => {
    const node = e.target;
    const connectedEdges = node.connectedEdges();
    const connectedNodes = connectedEdges.connectedNodes();

    cyInstance.elements().removeClass('highlighted dimmed show-label');
    cyInstance.elements().addClass('dimmed');
    node.removeClass('dimmed').addClass('highlighted');
    connectedNodes.removeClass('dimmed').addClass('highlighted');
    connectedEdges.removeClass('dimmed').addClass('highlighted show-label');

    const d = node.data();
    const side = document.getElementById('side');
    if (side) {
      const name = d.name || d.fullLabel || d.label || d.id;
      side.innerHTML = `<h3>${name}</h3><p>${d.id} &middot; ${d.type}</p><button class="pill c" onclick="if(window.pathFrom)pathFrom('${d.id}')">Trace a path</button>`;
    }

    if (options.onNodeTap) {
      options.onNodeTap(d);
    }
  });

  // On edge tap: show relationship and record_id chip in #side
  cyInstance.on('tap', 'edge', e => {
    const edge = e.target;
    cyInstance.elements().removeClass('highlighted dimmed show-label');
    cyInstance.elements().addClass('dimmed');
    edge.removeClass('dimmed').addClass('highlighted show-label');
    edge.connectedNodes().removeClass('dimmed').addClass('highlighted');

    const d = edge.data();
    const side = document.getElementById('side');
    if (side) {
      const rel = d.label || d.relationship || 'Relationship';
      const recId = d.rec || d.id || '';
      side.innerHTML = `<h3>${rel}</h3><p>${d.source} &rarr; ${d.target}</p>${recId ? `<span class="chip">${recId}</span>` : ''}`;
    }

    if (options.onEdgeTap) {
      options.onEdgeTap(d);
    }
  });

  // On background tap: clear highlighting
  cyInstance.on('tap', e => {
    if (e.target === cyInstance) {
      cyInstance.elements().removeClass('highlighted dimmed show-label');
      const side = document.getElementById('side');
      if (side) {
        side.innerHTML = '<p>Click a node or an edge.</p>';
      }
      if (options.onBackgroundTap) {
        options.onBackgroundTap();
      }
    }
  });

  // Observe container size changes (visibility / window resize / tab change)
  if (containerResizeObserver) {
    containerResizeObserver.disconnect();
  }
  if (window.ResizeObserver) {
    containerResizeObserver = new ResizeObserver(entries => {
      for (const entry of entries) {
        if (entry.contentRect.width > 0 && entry.contentRect.height > 0) {
          if (cyInstance) {
            cyInstance.resize();
            cyInstance.fit(undefined, 40);
          }
        }
      }
    });
    containerResizeObserver.observe(container);
  }

  // Run layout
  runCoseLayout(cyInstance);

  return cyInstance;
}

function fitGraph() {
  if (cyInstance) {
    cyInstance.resize();
    cyInstance.fit(undefined, 40);
  }
}

function relayoutGraph() {
  if (cyInstance) {
    runCoseLayout(cyInstance);
  }
}

function refreshGraph() {
  if (cyInstance) {
    const container = cyInstance.container();
    if (container && container.offsetWidth > 0 && container.offsetHeight > 0) {
      cyInstance.resize();
      cyInstance.fit(undefined, 40);
    }
  }
}

function updateGraphTheme() {
  if (!cyInstance) return;
  const colors = getThemeColors();
  cyInstance.batch(() => {
    cyInstance.style()
      .selector('node')
      .style({
        'border-color': colors.ink,
        'color': colors.ink,
        'text-background-color': colors.paper
      })
      .selector('node.is-start')
      .style({
        'border-color': colors.ink
      })
      .selector('edge')
      .style({
        'line-color': colors.ink,
        'target-arrow-color': colors.ink
      })
      .selector('edge.show-label')
      .style({
        'color': colors.ink,
        'text-background-color': colors.paper
      })
      .update();
  });
}

// Global window resize listener
let resizeTimer = null;
window.addEventListener('resize', () => {
  clearTimeout(resizeTimer);
  resizeTimer = setTimeout(() => {
    refreshGraph();
  }, 100);
});

// Global exports
window.drawGraph = drawGraph;
window.fitGraph = fitGraph;
window.relayoutGraph = relayoutGraph;
window.refreshGraph = refreshGraph;
window.updateGraphTheme = updateGraphTheme;
window.getNodeColor = getNodeColor;
