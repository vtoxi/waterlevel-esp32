document.addEventListener('DOMContentLoaded', () => {
  const navToggle = document.querySelector('.nav-toggle');
  const navList = document.querySelector('.nav-list');

  navToggle.addEventListener('click', () => {
    navToggle.classList.toggle('active');
    navList.classList.toggle('active');
  });

  // Close menu when clicking outside
  document.addEventListener('click', (e) => {
    if (!navToggle.contains(e.target) && !navList.contains(e.target)) {
      navToggle.classList.remove('active');
      navList.classList.remove('active');
    }
  });

  // Close menu when window is resized above mobile breakpoint
  window.addEventListener('resize', () => {
    if (window.innerWidth > 768) {
      navToggle.classList.remove('active');
      navList.classList.remove('active');
    }
  });

  // Generic AJAX handler for all forms ending with 'Form'
  document.querySelectorAll('form[id$="Form"]').forEach(function(form) {
    if (form.id === 'resetForm') {
      form.onsubmit = function(e) {
        if (!confirm('Are you sure you want to perform a factory reset? This will erase all settings and cannot be undone.')) return false;
        e.preventDefault();
        fetch(form.action || window.location.pathname, {
          method: 'POST'
        })
        .then(async r => { const msg = await r.text(); document.getElementById('deviceMsg').innerHTML = msg; })
        .catch(err => { document.getElementById('deviceMsg').innerHTML = '<span style="color:red;">' + err + '</span>'; });
        return false;
      };
    } else {
      form.onsubmit = function(e) {
        e.preventDefault();
        const data = new URLSearchParams(new FormData(form)).toString();
        fetch(form.action || window.location.pathname, {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: data
        })
        .then(async r => {
          const msg = await r.text();
          const msgDiv = document.getElementById(form.id.replace('Form', 'Msg'));
          if (msgDiv) msgDiv.innerHTML = msg;
        })
        .catch(err => {
          const msgDiv = document.getElementById(form.id.replace('Form', 'Msg'));
          if (msgDiv) msgDiv.innerHTML = '<span style="color:red;">' + err + '</span>';
        });
      };
    }
  });
}); 