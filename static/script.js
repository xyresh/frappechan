async function fetchPosts() {
    const response = await fetch('/posts');
    const posts = await response.json();
    const postsContainer = document.getElementById('posts');
    const postsMap = {};
    postsContainer.innerHTML = '';

    // Create a map of posts by id
    posts.forEach(post => {
        postsMap[post.id] = post;
        post.replies = [];
    });

    // Populate replies array for each post
    posts.forEach(post => {
        if (post.replyTo && postsMap[post.replyTo]) {
            postsMap[post.replyTo].replies.push(post);
        }
    });

    // Append top-level posts to the container
    Object.values(postsMap).forEach(post => {
        if (!post.replyTo) {
            postsContainer.appendChild(createPostElement(post));
        }
    });

    function createPostElement(post) {
        const postElement = document.createElement('div');
        postElement.className = 'post';
        postElement.innerHTML = `
            <p>αρ. δημ.: <a href="#" onclick="replyTo('${post.id}')">${post.id}</a> | ${post.timestamp}</p>
            <p>Απάντηση σε: ${post.replyTo ? post.replyTo : 'Ουδέν'}</p>
            <img class="toggle-size" src="../uploads/${post.imagePath}" alt="Image" style="max-width: 60%; height: auto;">
            <p>${post.content}</p>
            ${post.replies.length > 0 ? `<p class="reply-toggle">Εμφάνιση νήματος (${post.replies.length})</p>` : ''}
            <div class="reply-container"></div>
        `;

        // Add event listener for image toggle functionality
        const image = postElement.querySelector('img.toggle-size');
        if (image) {
            image.addEventListener('click', () => {
                image.classList.toggle('expanded');
            });
        }

        const replyContainer = postElement.querySelector('.reply-container');
        post.replies.forEach(reply => {
            replyContainer.appendChild(createPostElement(reply));
        });

        const replyToggle = postElement.querySelector('.reply-toggle');
        if (replyToggle) {
            replyToggle.addEventListener('click', () => {
                const isVisible = replyContainer.style.display === 'block';
                replyContainer.style.display = isVisible ? 'none' : 'block';
                replyToggle.textContent = isVisible ? `Εμφάνιση νήματος (${post.replies.length})` : `Απόκρυψη νήματος (${post.replies.length})`;
            });
        }

        return postElement;
    }
}

function replyTo(id) {
    const replyToField = document.getElementById('reply_to');
    replyToField.value = id;
}

function scrollToTop() {
    window.scrollTo({ top: 0, behavior: 'smooth' });
}

window.onload = fetchPosts;
