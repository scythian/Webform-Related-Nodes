<?php

/**
 * @file
 * Views handler to display data value of a webform submission component.
 */

/**
 * Field handler to show submission data.
 *
 * @ingroup views_field_handlers
 */
class views_handler_related_nodes extends views_handler_field {

  /**
   * Class constructor.
   */
  public function construct() {
    parent::construct();
  }

  /**
   * Handler option definition.
   *
   * @return array
   *   Options list.
   */
  public function option_definition() {
    $options = parent::option_definition();
    $options['custom_label'] = array('default' => 'default');
    $options['webform_nid'] = array('default' => NULL);
    $options['webform_cid'] = array('default' => NULL);
    $options['webform_if'] = array('default' => NULL);
    $options['webform_related_cp_fields'] = array('default' => NULL);
    return $options;
  }

  /**
   * Options form.
   *
   * @param array $form
   *   The form parameters array.
   * @param array $form_state
   *   The form state array.
   */
  public function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    form_load_include($form_state, 'inc', 'webform', 'views/webform.views');
    form_load_include($form_state, 'inc', 'webform', 'includes/webform.components');

    $form['custom_label']['#type'] = 'radios';
    $form['custom_label']['#options'] = array(
      'default' => t('Use component label'),
      'custom' => t('Custom label'),
      'none' => t('No label'),
    );
    $form['custom_label']['#default_value'] = $this->options['custom_label'];
    $form['label']['#dependency'] = array('radio:options[custom_label]' => array('custom'));

    $webform_nid = (int) $this->options['webform_nid'];
    $cid = (int) $this->options['webform_cid'];

    $node = $webform_nid ? node_load($webform_nid) : NULL;

    // Required webform.
    $form['webform_nid'] = array(
      '#type' => 'textfield',
      '#title' => t('Webform node'),
      '#default_value' => isset($node) ? $node->title . ' [nid:' . $node->nid . ']' : '',
      '#ajax' => array(
        'path' => views_ui_build_form_url($form_state),
        'event' => 'blur',
      ),
      '#autocomplete_path' => 'webform/autocomplete',
      '#description' => t('Enter the title or NID of the Webform whose values should be made available.'),
      '#submit' => array('views_ui_config_item_form_submit_temporary'),
      '#executes_submit_callback' => TRUE,
    );

    // Detect related component for selected above webform.
    $query = db_select('webform_component', 'w');
    $query->fields('w', array(
      'cid',
      'name',
      'extra',
    ));
    $query->condition('w.type', 'related_nodes');
    $query->condition('w.nid', $webform_nid);
    $query->orderBy('w.name', 'ASC');
    $result = $query->execute();

    $content_types = array();
    foreach ($result as $row) {
      $extra = unserialize($row->extra);
      $content_types = array_merge($content_types, array_filter($extra['content_types']));
      $related_nodes_options[$row->cid] = $row->name;
    }

    $form['webform_cid'] = array(
      '#title' => t('Related component'),
      '#type' => 'select',
      '#options' => $related_nodes_options,
      '#default_value' => $cid,
      '#access' => count($related_nodes_options),
      '#description' => t('Select the component whose values should be made available.'),
    );

    // Content type field list.
    $content_type_fields = $this->getContentTypeFields($content_types);
    $form['webform_related_cp_fields'] = array(
      '#title' => t('Field for rendering'),
      '#type' => 'select',
      '#options' => $content_type_fields,
      '#access' => count($content_type_fields),
      '#default_value' => $this->options['webform_related_cp_fields'],
      '#description' => t('Select field to display.'),
    );

  }

  /**
   * Return Content Type Fields List.
   *
   * @param array $content_type
   *   Neddle content type.
   *
   * @return array
   *   Field list for content type.
   */
  public function getContentTypeFields($content_type = array()) {
    $options = array();

    if (!empty($content_type)) {
      // Select content type field list.
      $query = db_select('field_config_instance', 'fci');
      $query->innerJoin('node_type', 'nt', 'nt.name=fci.bundle');
      $query->fields('fci', array('field_name', 'data'));
      $query->fields('nt', array('type', 'name'));
      $query->condition('fci.bundle', $content_type);
      $query->orderBy('nt.name', 'ASC');
      $query->orderBy('fci.field_name', 'ASC');
      $result = $query->execute();

      // Process content type fields list.
      $node_labels = array();
      foreach ($result as $row) {
        if (!$node_labels[$row->name]) {
          $node_labels[$row->type] = $row->name;
        }

        $data = unserialize($row->data);

        if ($options[$row->field_name]) {
          $options[$row->field_name] = $row->name . ', ' . $options[$row->field_name];
        }
        else {
          $options[$row->field_name] = $row->name . ' : ' . $row->field_name . ' (' . $data['label'] . ') ';
        }
      }

      // Add hardcoded title field in the top of list.
      if (!empty($node_labels)) {
        $options = array_merge(array(
          'title' => implode(', ', $node_labels) . ' : title (Title)',
        ), $options);
      }
    }

    return $options;
  }

  /**
   * Custom options validate handler.
   *
   * @param array $form
   *   The form parameters array.
   * @param array $form_state
   *   The form state array.
   */
  function options_validate(&$form, &$form_state) {
    parent::options_validate($form, $form_state);
    _webform_views_options_validate($form, $form_state);
  }

  /**
   * Custom options submit handler.
   *
   * @param array $form
   *   The form parameters array.
   * @param array $form_state
   *   The form state array.
   */
  function options_submit(&$form, &$form_state) {
    parent::options_submit($form, $form_state);
    _webform_views_options_submit($form, $form_state);
  }

  /**
   * Load the node and submissions needed for this components values.
   *
   * @param array $values
   *   Submitted values array.
   */
  public function pre_render(&$values) {
    $webform_nid = $this->options['webform_nid'];
    $this->webform_node = node_load($webform_nid);
    // Load all the submissions needed for this page. This is stored at the
    // view level to ensure it's available between fields so we don't load
    // them twice.
    if (!isset($this->view->_webform_submissions[$webform_nid])) {
      module_load_include('inc', 'webform', 'includes/webform.submissions');
      $this->view->_webform_submissions[$webform_nid] = array();
      $sids = array();
      foreach ($values as $value) {
        $sids[] = $value->{$this->field_alias};
      }
      if ($sids) {
        $this->view->_webform_submissions[$webform_nid] = webform_get_submissions(array('sid' => $sids));
      }
    }
  }

  /**
   * Render the field using the loaded submissions from pre_render().
   */
  public function render($row) {
    $output = array();
    $sid = $this->get_value($row);
    $webform_nid = $this->options['webform_nid'];
    $cid = $this->options['webform_cid'];
    $field = $this->options['webform_related_cp_fields'];
    $webform = $this->webform_node;

    if (isset($sid) && isset($webform->webform['components'][$cid])) {
      $submission = $this->view->_webform_submissions[$webform_nid][$sid];
      $related_nids = array_filter($submission->data[$cid]);
      foreach ($related_nids as $key => $nid) {
        // Cache identifier.
        $cache_id = 'handler_related_nodes' . $submission->data[$cid][$key];

        // Get node instance.
        if ($cache = cache_get($cache_id)) {
          $node = $cache->data;
        }
        else {
          $node = node_load($nid);
          cache_set($cache_id, $node, 'cache', REQUEST_TIME + 5);
        }

        if ($field == 'title') {
          $output[] = $node->title;
        }
        else {
          // Get the instance for display settings.
          $instance = field_read_instance('node', $field, $node->type);
          if ($instance) {
            // Prepare field for render, using webform display.
            $output[] = drupal_render(field_view_field('node', $node, $field, $instance['display']['webform_submissions_view']));
          }
        }
      }
    }

    return implode('<br/>', $output);
  }
}
