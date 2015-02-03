<?php

/**
 * @file
 * Views handler to calculate node submissions.
 */

/**
 * Field handler to show submission data.
 *
 * @ingroup views_field_handlers
 */
class views_handler_related_nodes_count extends views_handler_field {

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

    // Detect related components for selected above webform.
    $components = _webform_related_nodes_get_related_components($webform_nid);
    $content_types = array();
    $related_nodes_options = array();
    foreach ($components as $row) {
      $extra = unserialize($row->extra);
      $content_types = array_merge($content_types, array_filter($extra['content_types']));
      $related_nodes_options[$row->cid] = $row->name;
    }

    $form['webform_cid'] = array(
      '#title' => t('Related component'),
      '#type' => 'checkboxes',
      '#options' => $related_nodes_options,
      '#default_value' => $this->options['webform_cid'],
      '#access' => count($related_nodes_options),
      '#description' => t('Select the component whose values should be made available.'),
    );

  }

  /**
   * Custom options validate handler.
   *
   * @param array $form
   *   The form parameters array.
   * @param array $form_state
   *   The form state array.
   */
  public function options_validate(&$form, &$form_state) {
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
  public function options_submit(&$form, &$form_state) {
    parent::options_submit($form, $form_state);
    _webform_views_options_submit($form, $form_state);
  }

  /**
   * Called to add the field to a query.
   */
  public function query($group_by = FALSE) {
    /* No query to run */
  }

  /**
   * Render the field using the loaded submissions from pre_render().
   */
  public function render($values) {
    // Calculate submissions for node.
    $count = db_select('webform_submitted_data', 'd')
      ->fields(NULL, array('field'))
      ->condition('d.nid', $this->options['webform_nid'])
      ->condition('d.cid', $this->options['webform_cid'])
      ->condition('d.data', $values->nid)
      ->countQuery()
      ->execute()
      ->fetchField();

    return $count;
  }
}
